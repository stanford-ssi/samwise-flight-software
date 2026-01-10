/*
FTP for sending specific packets over 2400
*/

#include "SX128x_Linux.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

constexpr size_t PACKET_SIZE = 253;

std::vector<int> getPackets(std::string str, int filesize)
{
    std::vector<int> vec;
    std::string buf = "";

    for (const auto &c : str)
    {
        if (c == ',')
        {
            vec.push_back(stoi(buf));
            buf = "";
        }
        else
        {
            if ((c >= 48) && (c <= 58))
            {
                buf += c;
            }
        }
    }
    if (buf != "")
    {
        vec.push_back(stoi(buf));
    };

    // Exclude out of bounds packets
    std::vector<int> packets;
    for (int packetnum : vec)
    {
        if (packetnum * PACKET_SIZE < filesize)
        {
            packets.push_back(packetnum);
        }
        else
        {
            puts("Packet is out of bounds");
        }
    }
    return packets;
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        printf("Usage: Lora_tx_packets <freq in MHz> <file to send> <packets "
               "to send>\n");
        return 1;
    }

    // Pins based on hardware configuration
    SX128x_Linux Radio("/dev/spidev0.0", 0, {27, 26, 20, 16, -1, -1, 24, 25});

    // Assume we're running on a high-end Raspberry Pi,
    // so we set the SPI clock speed to the maximum value supported by the chip
    Radio.SetSpiSpeed(8000000);

    Radio.Init();
    Radio.SetStandby(SX128x::STDBY_XOSC);
    Radio.SetRegulatorMode(static_cast<SX128x::RadioRegulatorModes_t>(0));
    Radio.SetLNAGainSetting(SX128x::LNA_HIGH_SENSITIVITY_MODE);
    Radio.SetTxParams(0, SX128x::RADIO_RAMP_20_US);
    Radio.SetBufferBaseAddresses(0x00, 0x00);

    // Modulation Parameters
    SX128x::ModulationParams_t ModulationParams;
    ModulationParams.PacketType = SX128x::PACKET_TYPE_LORA;
    ModulationParams.Params.LoRa.CodingRate = SX128x::LORA_CR_4_8;
    ModulationParams.Params.LoRa.Bandwidth = SX128x::LORA_BW_1600;
    ModulationParams.Params.LoRa.SpreadingFactor = SX128x::LORA_SF7;

    // Packet Parameters
    SX128x::PacketParams_t PacketParams;
    PacketParams.PacketType = SX128x::PACKET_TYPE_LORA;
    auto &l = PacketParams.Params.LoRa;
    l.PayloadLength = PACKET_SIZE;
    l.HeaderType = SX128x::LORA_PACKET_FIXED_LENGTH;
    l.PreambleLength = 12;
    l.Crc = SX128x::LORA_CRC_ON;
    l.InvertIQ = SX128x::LORA_IQ_NORMAL;

    // Setting all up
    Radio.SetPacketType(SX128x::PACKET_TYPE_LORA);
    Radio.SetModulationParams(ModulationParams);
    Radio.SetPacketParams(PacketParams);

    auto freq = strtol(argv[1], nullptr, 10);
    Radio.SetRfFrequency(freq * 1000000UL);

    // TX done interrupt handler
    Radio.callbacks.txDone = []
    {
        // puts("Done!");
    };

    auto IrqMask = SX128x::IRQ_TX_DONE | SX128x::IRQ_RX_TX_TIMEOUT;
    Radio.SetDioIrqParams(IrqMask, IrqMask, SX128x::IRQ_RADIO_NONE,
                          SX128x::IRQ_RADIO_NONE);
    Radio.StartIrqHandler();

    auto pkt_ToA = Radio.GetTimeOnAir();

    // Open file
    std::ifstream file = std::ifstream(argv[2], std::ios::binary);
    file.seekg(0, std::ios::end);
    size_t filesize = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<int> packets = getPackets(argv[3], filesize);

    // Sending beginning packet with filesize
    printf("Sending %d packets (%d bytes)...\n", packets.size(),
           packets.size() * PACKET_SIZE);
    std::string numBytes = std::to_string(packets.size() * PACKET_SIZE) + "\0";
    Radio.SendPayload((uint8_t *)numBytes.data(), numBytes.size(),
                      {SX128x::RADIO_TICK_SIZE_1000_US, 1000});
    usleep((pkt_ToA + 20) * 2'000);

    // Sending all packets
    for (int packetnum : packets)
    {
        char buffer[PACKET_SIZE];
        file.seekg(packetnum * PACKET_SIZE, std::ios::beg);
        file.read(buffer, PACKET_SIZE);
        Radio.SendPayload(buffer, PACKET_SIZE,
                          {SX128x::RADIO_TICK_SIZE_1000_US, 1000});
        printf("Sent packet %d with size %d\n", packetnum, PACKET_SIZE);
        usleep((pkt_ToA + 20) * 1000);
    }

    printf("Done!, Now exiting...\n");
    file.close();
    Radio.StopIrqHandler();
    return EXIT_SUCCESS;
}
