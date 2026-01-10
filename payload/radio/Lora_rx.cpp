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

#include <cstdio>
#include <cstring>

#include <fstream>
#include <iostream>
#include <string>

#define PACKET_SIZE 253

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("Usage: Lora_rx <freq in MHz> <file to receive>\n");
        return 1;
    }

    // Pins based on hardware configuration
    SX128x_Linux Radio("/dev/spidev0.0", 0,
                       {26, 22, 5, 19, -1, -1, 23, 24, 25});

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

    Radio.SetBufferBaseAddresses(0x00, 0x00);
    puts("SetBufferBaseAddresses done");

    SX128x::ModulationParams_t ModulationParams;
    SX128x::PacketParams_t PacketParams;

    ModulationParams.PacketType = SX128x::PACKET_TYPE_LORA;
    ModulationParams.Params.LoRa.CodingRate = SX128x::LORA_CR_4_8;
    ModulationParams.Params.LoRa.Bandwidth = SX128x::LORA_BW_1600;
    ModulationParams.Params.LoRa.SpreadingFactor = SX128x::LORA_SF7;

    PacketParams.PacketType = SX128x::PACKET_TYPE_LORA;
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

    std::cout << "Firmware Version: " << Radio.GetFirmwareVersion() << "\n";

    // Open file
    std::ofstream fileToReceive(argv[2], std::ios::binary);

    size_t packetCount = 0;

    size_t expectedBytes = 0;
    size_t expectedFullPackets = 0;
    size_t partialPacketBytes = 0;

    bool receivedFirstPacket = false;
    bool done = false;

    Radio.callbacks.rxDone = [&]
    {
        printf("Received packet -> ");
        SX128x::PacketStatus_t ps;
        Radio.GetPacketStatus(&ps);

        uint8_t recv_buf[PACKET_SIZE];
        uint8_t recv_size;
        Radio.GetPayload(recv_buf, &recv_size, PACKET_SIZE);

        printf("%d bytes ", recv_size);

        // If first packet, set size
        if (!receivedFirstPacket)
        {
            expectedBytes = atoi((const char *)recv_buf);

            partialPacketBytes = expectedBytes % PACKET_SIZE;
            expectedFullPackets =
                (expectedBytes - partialPacketBytes) / PACKET_SIZE;

            printf("Expecting %d bytes =>", expectedBytes);
            printf("(%d full packets, %d partial bytes)\n", expectedFullPackets,
                   partialPacketBytes);

            receivedFirstPacket = true;
            return;
        }

        // Other packets -> write to file
        char *rev_buf_char = (char *)&recv_buf[0];
        packetCount++;

        if (packetCount > expectedFullPackets)
        {
            // Receiving partial last packet
            fileToReceive.write(rev_buf_char, partialPacketBytes);
            puts("Received last partial packet!");
            done = true;
            return;
        }

        // Normal full packet
        fileToReceive.write(rev_buf_char, recv_size);

        printf("packet count: %ld\n", packetCount);

        // Print packet info
        int8_t noise = ps.LoRa.RssiPkt - ps.LoRa.SnrPkt;
        int8_t rscp = ps.LoRa.RssiPkt + ps.LoRa.SnrPkt;
        printf("recvd %u bytes, RSCP: %d, RSSI: %d, Noise: %d, SNR: %d\n",
               recv_size, rscp, ps.LoRa.RssiPkt, noise, ps.LoRa.SnrPkt);
    };

    auto IrqMask =
        SX128x::IRQ_RX_DONE | SX128x::IRQ_TX_DONE | SX128x::IRQ_RX_TX_TIMEOUT;
    Radio.SetDioIrqParams(IrqMask, IrqMask, SX128x::IRQ_RADIO_NONE,
                          SX128x::IRQ_RADIO_NONE);
    puts("SetDioIrqParams done");

    Radio.StartIrqHandler();
    puts("StartIrqHandler done");

    Radio.SetRx({SX128x::RADIO_TICK_SIZE_1000_US, 0xFFFF});
    puts("SetRx done");

    while (!done)
    {
        // Wait until lambda says we are done
        sleep(1);
    }

    printf("Done!\n");
    fileToReceive.close();
    Radio.StopIrqHandler();
    exit(EXIT_SUCCESS);
}
