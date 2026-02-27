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
#include <iostream>

SX128x_Linux::SX128x_Linux(const std::string &spi_dev_path,
                           uint16_t gpio_dev_num,
                           SX128x_Linux::PinConfig pin_config)
    : pin_cfg(pin_config),
      RadioSpi(spi_dev_path, SPI_MODE_0 | SPI_NO_CS, 8, 500000),
      RadioGpio(gpio_dev_num),
      Busy(RadioGpio.line(pin_cfg.busy, GPIO::LineMode::Input, 0,
                          "SX128x BUSY")),
      RadioNss(
          RadioGpio.line(pin_cfg.nss, GPIO::LineMode::Output, 1, "SX128x NSS")),
      RadioReset(RadioGpio.line(pin_cfg.nrst, GPIO::LineMode::Output, 1,
                                "SX128x NRESET"))
{
    if (pin_config.tx_en >= 0)
    {
        TxEn = RadioGpio.line(pin_cfg.tx_en, GPIO::LineMode::Output, 0,
                              "SX128x TXEN");
    }

    if (pin_config.rx_en >= 0)
    {
        RxEn = RadioGpio.line(pin_cfg.rx_en, GPIO::LineMode::Output, 0,
                              "SX128x RXEN");
    }

    if (pin_config.tcxo >= 0)
    {
        TCXO = RadioGpio.line(pin_cfg.tcxo, GPIO::LineMode::Output, 1,
                              "SX128x TCXO_EN");
    }

    int i = 1;
    for (auto it : {pin_config.dio1, pin_config.dio2, pin_config.dio3})
    {
        std::string label = "SX128x DIO";
        label += std::to_string(i);
        i++;

        if (it != -1)
        {
            RadioGpio.on_event(
                it, GPIO::LineMode::Input, GPIO::EventMode::RisingEdge,
                [this](GPIO::EventType t, uint64_t)
                {
                    if (t == GPIO::EventType::RisingEdge)
                        ProcessIrqs();
                },
                label);
        }
    }
}

void SX128x_Linux::SetSpiSpeed(uint32_t hz)
{
    RadioSpi.set_max_speed_hz(hz);
}

void SX128x_Linux::SetExternalLock(std::mutex &m)
{
    ExtLock = &m;
}

void SX128x_Linux::StartIrqHandler(int __prio)
{
    IrqThread = std::thread(
        [this, __prio]()
        {
            sched_param param;
            param.sched_priority = __prio;
            pthread_setschedparam(pthread_self(), SCHED_RR, &param);
            RadioGpio.run_eventlistener();
        });
}

void SX128x_Linux::StopIrqHandler()
{
    RadioGpio.stop_eventlistener();
    IrqThread.join();
}

uint8_t SX128x_Linux::HalGpioRead(SX128x::GpioPinFunction_t func)
{
    switch (func)
    {
        case SX128x::GPIO_PIN_BUSY:
            return Busy.read();
        default:
            return 0;
    }
}

void SX128x_Linux::HalGpioWrite(SX128x::GpioPinFunction_t func, uint8_t value)
{
    switch (func)
    {
        case SX128x::GPIO_PIN_RESET:
            RadioReset.write(value);
        default:
            return;
    }
}

void SX128x_Linux::HalSpiTransfer(uint8_t *buffer_in, const uint8_t *buffer_out,
                                  uint16_t size)
{
    if (ExtLock)
    {
        std::lock_guard<std::mutex> lg(*ExtLock);

        RadioNss.write(0);
        RadioSpi.transfer(buffer_out, buffer_in, size);
        RadioNss.write(1);
    }
    else
    {
        RadioNss.write(0);
        RadioSpi.transfer(buffer_out, buffer_in, size);
        RadioNss.write(1);
    }

    // --- START DEBUG PRINTING ---
    /*
        printf("SPI Transaction (Len %d):\n", size);

        printf("  TX: ");
        if (buffer_out) {
            for (int i = 0; i < size; i++) {
                printf("%02X ", buffer_out[i]);
            }
        }
        printf("\n");

        printf("  RX: ");
        if (buffer_in) {
            for (int i = 0; i < size; i++) {
                printf("%02X ", buffer_in[i]);
            }
        }
        printf("\n------------------\n");
    */
    if (buffer_out && size > 0)
    {
        uint8_t opcode = buffer_out[0];

        // Check if the opcode is WriteRegister (0x18) or WriteBuffer (0x1A)
        if (opcode == 0x18 || opcode == 0x1A)
        {

            // Check if TCXO was initialized
            if (TCXO)
            {
                // Read and print the current state of the TCXO pin
                printf("WRITE OP (0x%02X) - TCXO Pin Value: %d\n", opcode,
                       TCXO->read());
            }
        }
    } // --- END DEBUG PRINTING ---
}

void SX128x_Linux::HalPreTx()
{
    if (TxEn)
    {
        TxEn->write(1);
    }
}

void SX128x_Linux::HalPreRx()
{
    if (RxEn)
    {
        RxEn->write(1);
    }
}

void SX128x_Linux::HalPostTx()
{
    if (TxEn)
    {
        TxEn->write(0);
    }
}

void SX128x_Linux::HalPostRx()
{
    if (RxEn)
    {
        RxEn->write(0);
    }
}
