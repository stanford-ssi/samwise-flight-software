/*
    This file is part of SX128x Linux driver.
    Copyright (C) 2020 ReimuNotMoe

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
constexpr size_t HEADER_LEN =
    0; // currently not using the header, might become useful in the future

using Packet = std::vector<uint8_t>;

class PacketizedFile
{

public:
    std::string filepath;
    size_t filesize;
    std::ifstream file;
    size_t numpackets;
    size_t currentpacket_id;

    PacketizedFile(std::string fpath)
    {
        filepath = fpath;
        file = std::ifstream(filepath, std::ios::binary);
        file.seekg(0, std::ios::end);
        filesize = file.tellg();
        file.seekg(0, std::ios::beg);

        currentpacket_id = 0;
        numpackets = ceil(static_cast<float>(filesize) / PACKET_SIZE);
    }

    Packet calculate_header(const Packet &packet, uint16_t id = 0)
    {
        // Currently an empty function, in case we might need to add a header in
        // the future.
        Packet header;
        return header;
    }

    Packet getNextPacket()
    {
        Packet packet(PACKET_SIZE, 0); // Initialize with PACKET_SIZE zeros
        file.read(reinterpret_cast<char *>(packet.data()), PACKET_SIZE);
        size_t N =
            (currentpacket_id != numpackets - 1 ? PACKET_SIZE
                                                : filesize % PACKET_SIZE);
        packet.resize(N);
        Packet finalpacket = calculate_header(packet, currentpacket_id);
        finalpacket.insert(finalpacket.end(), packet.begin(), packet.end());
        return finalpacket;
    }

    class Iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Packet;
        using pointer = Packet *;
        using reference = Packet &;

        Iterator(PacketizedFile *pt, bool beginning = false)
        {
            p = pt;
            ptr = &currentpacket;
            if (beginning)
            { // If we're initialing the begin() iterator, we also generate the
              // first Packet
                operator++();
            }
        }
        reference operator*() const
        {
            return *ptr;
        }
        pointer operator->()
        {
            return ptr;
        }

        Iterator &operator++()
        {
            currentpacket = p->getNextPacket();

            p->currentpacket_id++;
            return *this;
        }

        bool operator!=(const Iterator &b)
        {
            if (p->currentpacket_id > p->numpackets)
            {
                printf("File Closed");
                p->file.close();
            }

            return (p->currentpacket_id <= p->numpackets);
        }

    private:
        pointer ptr;
        PacketizedFile *p;
        Packet currentpacket;
    };

    Iterator begin()
    {
        return Iterator(this, true);
    }
    Iterator end()
    {
        return Iterator(this);
    }
};

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("Usage: Lora_tx <freq in MHz> <file to send>\n");
        return 1;
    }

    // Pins based on hardware configuration
    SX128x_Linux Radio("/dev/spidev0.0", 0, {26, 22, 5, 19, -1, -1, 23, 24});

    // Assume we're running on a high-end Raspberry Pi,
    // so we set the SPI clock speed to the maximum value supported by the chip
    Radio.SetSpiSpeed(8000000);

    Radio.Init();
    puts("Init done");
    Radio.SetStandby(SX128x::STDBY_XOSC);
    puts("SetStandby done");
    Radio.SetRegulatorMode(static_cast<SX128x::RadioRegulatorModes_t>(0));
    puts("SetRegulatorMode done");
    Radio.SetLNAGainSetting(SX128x::LNA_HIGH_SENSITIVITY_MODE);
    puts("SetLNAGainSetting done");
    Radio.SetTxParams(0, SX128x::RADIO_RAMP_20_US);
    puts("SetTxParams done");

    Radio.SetBufferBaseAddresses(0x00, 0x00);
    puts("SetBufferBaseAddresses done");

    SX128x::ModulationParams_t ModulationParams;
    SX128x::PacketParams_t PacketParams;

    // Modulation Parameters
    ModulationParams.PacketType = SX128x::PACKET_TYPE_LORA;
    ModulationParams.Params.LoRa.CodingRate = SX128x::LORA_CR_4_8;
    ModulationParams.Params.LoRa.Bandwidth = SX128x::LORA_BW_1600;
    ModulationParams.Params.LoRa.SpreadingFactor = SX128x::LORA_SF7;

    PacketParams.PacketType = SX128x::PACKET_TYPE_LORA;

    // Packet Parameters
    auto &l = PacketParams.Params.LoRa;
    l.PayloadLength = PACKET_SIZE;
    l.HeaderType = SX128x::LORA_PACKET_FIXED_LENGTH;
    l.PreambleLength = 12;
    l.Crc = SX128x::LORA_CRC_ON;
    l.InvertIQ = SX128x::LORA_IQ_NORMAL;

    Radio.SetPacketType(SX128x::PACKET_TYPE_LORA);

    puts("SetPacketType done");
    Radio.SetModulationParams(ModulationParams);
    puts("SetModulationParams done");
    Radio.SetPacketParams(PacketParams);
    puts("SetPacketParams done");

    auto freq = strtol(argv[1], nullptr, 10);
    Radio.SetRfFrequency(freq * 1000000UL);
    puts("SetRfFrequency done");

    std::cout << "Firmware version: " << Radio.GetFirmwareVersion() << "\n";

    // TX done interrupt handler
    Radio.callbacks.txDone = [] { puts("Done!"); };

    auto IrqMask = SX128x::IRQ_TX_DONE | SX128x::IRQ_RX_TX_TIMEOUT;
    Radio.SetDioIrqParams(IrqMask, IrqMask, SX128x::IRQ_RADIO_NONE,
                          SX128x::IRQ_RADIO_NONE);
    puts("SetDioIrqParams done");

    Radio.StartIrqHandler();
    puts("StartIrqHandler done");

    auto pkt_ToA = Radio.GetTimeOnAir();

    // Create packetized file
    PacketizedFile packetizedFile(argv[2]);

    // Sending beginning packet with filesize
    printf("Sending %d packets (%d bytes)...\n", packetizedFile.numpackets,
           packetizedFile.filesize);
    std::string numBytes = std::to_string(packetizedFile.filesize) + "\0";
    Radio.SendPayload((uint8_t *)numBytes.data(), numBytes.size(),
                      {SX128x::RADIO_TICK_SIZE_1000_US, 1000});
    usleep((pkt_ToA + 20) * 1'000);
    usleep(1'000);

    for (Packet &packet : packetizedFile)
    {
        Radio.SendPayload(packet.data(), packet.size(),
                          {SX128x::RADIO_TICK_SIZE_1000_US, 1000});
        printf("Sent packet %d with size %d\n",
               packetizedFile.currentpacket_id - 1, packet.size());
        usleep((pkt_ToA + 20) * 1000);
    }
    printf("Done!, Now exiting...\n");
    Radio.StopIrqHandler();
    return EXIT_SUCCESS;
}
