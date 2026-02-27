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
#pragma once

#include <SX128x.hpp>
#include <GPIO++.hpp>
#include <SPPI.hpp>

#include <string>
#include <thread>
#include <optional>

#include <cinttypes>

using namespace YukiWorkshop;

class SX128x_Linux : public SX128x {
public:
	struct PinConfig {
		int16_t busy = -1, nrst = -1, nss = -1, dio1 = -1, dio2 = -1, dio3 = -1;
		int16_t tx_en = -1, rx_en = -1, tcxo = -1;
	};

	SX128x_Linux(const std::string& spi_dev_path, uint16_t gpio_dev_num, PinConfig pin_config);

	// For sync with multiple instances
	void SetExternalLock(std::mutex& m);

	void StartIrqHandler(int __prio = 50);

	void StopIrqHandler();

	void SetSpiSpeed(uint32_t hz);

private:
	PinConfig pin_cfg;

	std::mutex* ExtLock = nullptr;

	std::thread IrqThread;

	SPPI RadioSpi;
	GPIO::Device RadioGpio;

	GPIO::LineSingle RadioNss;
	GPIO::LineSingle RadioReset;
	GPIO::LineSingle Busy;
	std::optional<GPIO::LineSingle> TxEn, RxEn, TCXO;

	uint8_t HalGpioRead(GpioPinFunction_t func) override;

	void HalGpioWrite(GpioPinFunction_t func, uint8_t value) override;

	void HalSpiTransfer(uint8_t *buffer_in, const uint8_t *buffer_out, uint16_t size) override;

	void HalPreTx() override;

	void HalPreRx() override;

	void HalPostTx() override;

	void HalPostRx() override;

};
