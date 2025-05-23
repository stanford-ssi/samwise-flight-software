#include "mppt.h"

// Configuration Register Data (mirrors Python CFG tuple)
const lt8491_cfg_register_t CFG[9] = {
    {"CFG_RSENSE1", 0x28, 0x2710}, {"CFG_RIMON_OUT", 0x2A, 0x0BC2},
    {"CFG_RSENSE2", 0x2C, 0x0CE4}, {"CFG_RDACO", 0x2E, 0x3854},
    {"CFG_RFBOUT1", 0x30, 0x0604}, {"CFG_RFBOUT2", 0x32, 0x0942},
    {"CFG_RDACI", 0x34, 0x0728},   {"RFBIN2", 0x36, 0x02DC},
    {"RDBIN1", 0x38, 0x03B9}};

mppt_t mppt_mk_mock()
{
    mppt_t device;
    device.i2c = NULL;
    device.address = 0x00;
    device.is_charging = false;
    device.is_initialized = true;
    device.VIN_mV = 123;
    device.charging_mV = 420;
    device.charging_mA = 1000;
    device.battery_mV = 4200;
    device.battery_mA = 20000;
    return device;
}

mppt_t mppt_mk(i2c_inst_t *i2c, uint8_t address)
{
    mppt_t device;
    device.i2c = i2c;
    device.address = address;
    device.is_charging = false;
    device.is_initialized = false;
    device.VIN_mV = 0;
    device.charging_mV = 0;
    device.charging_mA = 0;
    device.battery_mV = 0;
    device.battery_mA = 0;
    return device;
}

// --- Helper Functions for I2C Communication ---

// Writes a command (typically register address) and reads N bytes
int i2c_write_then_read(uint8_t device_addr, uint8_t *cmd_buf, size_t cmd_len,
                        uint8_t *read_buf, size_t read_len)
{
    int ret = i2c_write_blocking(SAMWISE_MPPT_I2C, device_addr, cmd_buf,
                                 cmd_len, true); // true for nostop
    if (ret < 0)
    {
        LOG_ERROR("Error writing command: %d\n", ret);
        return ret;
    }
    ret = i2c_read_blocking(SAMWISE_MPPT_I2C, device_addr, read_buf, read_len,
                            false);
    if (ret < 0)
    {
        LOG_ERROR("Error reading data: %d\n", ret);
    }
    return ret; // Number of bytes read or error code
}

// Writes a command buffer (e.g. register address + data)
int i2c_write_data(uint8_t device_addr, uint8_t *write_buf, size_t write_len)
{
    int ret = i2c_write_blocking(SAMWISE_MPPT_I2C, device_addr, write_buf,
                                 write_len, false);
    if (ret < 0)
    {
        LOG_ERROR("Error writing data: %d\n", ret);
    }
    return ret; // Number of bytes written or error code
}

uint16_t mppt_send_instruction_and_read_2_byte(mppt_t *device, uint8_t inst)
{
    if (!device->i2c)
    {
        return 0xFFFFFF; // No-op for mock device - simulate error code
    }
    uint8_t result_2_bytes[2];
    i2c_write_then_read(device->address, &inst, 1, result_2_bytes, 2);
    uint16_t tele_value_16 =
        (uint16_t)result_2_bytes[0] | ((uint16_t)result_2_bytes[1] << 8);
    return tele_value_16;
}

void mppt_init(mppt_t *device)
{
    // Initialize the I2C device
    if (device->is_initialized)
    {
        return; // Already initialized
    }

    // --- Configure LT8491 Registers ---
    uint8_t cmd_buf[2];
    uint8_t result_byte_array[1];

    cmd_buf[0] = LT8491_CTRL_CHRG_EN; // Register address for CTRL_CHRG_EN
    cmd_buf[1] = 0; // Value to disable charging (assuming 0 means disable)
    i2c_write_data(device->address, cmd_buf, 2);
    device->is_charging = false;

    LOG_INFO("Configuring LT8491 registers...\n");
    for (int i = 0; i < NUM_CFG_REGISTERS; ++i)
    {
        uint8_t write_buf[3];
        uint8_t read_cmd;
        uint8_t read_buf_word[2];
        uint16_t read_value_check;

        // LT8491 writes: RegAddr, LSB, MSB (if it's a single transaction for
        // 16-bit) The Python code does:
        //   i2c.writeto(LT8491_ADDR, bytearray([addr, low_byte]))
        //   i2c.writeto(LT8491_ADDR, bytearray([addr+1, high_byte]))
        // This means writing LSB to addr, and MSB to addr+1.

        cmd_buf[0] = CFG[i].addr;         // Register address for LSB
        cmd_buf[1] = CFG[i].value & 0xFF; // Low byte
        i2c_write_data(device->address, cmd_buf, 2);

        cmd_buf[0] = CFG[i].addr + 1;            // Register address for MSB
        cmd_buf[1] = (CFG[i].value >> 8) & 0xFF; // High byte
        i2c_write_data(device->address, cmd_buf, 2);

        // Read back the 16-bit value (LSB from CFG[i].addr, MSB from
        // CFG[i].addr+1)
        read_cmd = CFG[i].addr;
        read_value_check =
            mppt_send_instruction_and_read_2_byte(device, read_cmd);

        LOG_INFO("%s : Set 0x%04X, Read 0x%04X (LSB:0x%02X MSB:0x%02X)\n",
                 CFG[i].name, CFG[i].value, read_value_check, read_buf_word[0],
                 read_buf_word[1]);
    }
    LOG_INFO("LT8491 configuration complete.\n");

    // Turn ON charging after configuration
    // Python: i2c.writeto_then_readfrom(LT8491_ADDR, bytearray([0x23, 1]),
    // result)
    cmd_buf[0] = LT8491_CTRL_CHRG_EN; // CTRL_CHRG_EN register
    cmd_buf[1] = 1;                   // Value to enable
    i2c_write_data(device->address, cmd_buf, 2);
    device->is_charging = true;

    // Device completed initialization
    device->is_initialized = true;
}

// Read TELE_VBAT (Battery Voltage)
uint16_t mppt_get_battery_voltage(mppt_t *device)
{
    if (!device->i2c)
    {
        return device->battery_mV; // Mock device
    }
    // Voltage values are read as 100*V (so 10mV increments)
    uint16_t tele_value_16 =
        mppt_send_instruction_and_read_2_byte(device, LT8491_TELE_VBAT);
    uint16_t voltage_mV = tele_value_16 * 10; // Convert to mV
    LOG_DEBUG("TELE_VBAT: %u\n", tele_value_16);
    device->battery_mV = voltage_mV; // Store in device struct
    return device->battery_mV;
}

// Read TELE_IOUT (Battery Output Current)
uint16_t mppt_get_battery_current(mppt_t *device)
{
    if (!device->i2c)
    {
        return device->battery_mA; // Mock device
    }
    // Current values are read in mA directly
    uint16_t tele_value_16 =
        mppt_send_instruction_and_read_2_byte(device, LT8491_TELE_IOUT);
    LOG_DEBUG("TELE_IIN: %u\n", tele_value_16);
    device->battery_mA = tele_value_16; // Store in device struct
    return device->battery_mA;
}

// Read TELE_VINR (Charging Input Voltage - from solar panels)
uint16_t mppt_get_voltage(mppt_t *device)
{
    if (!device->i2c) // Mock device
    {
        return device->charging_mV;
    }
    // Voltage values are read in 100*V (so 10mV increments)
    uint16_t tele_value_16 =
        mppt_send_instruction_and_read_2_byte(device, LT8491_TELE_VINR);
    uint16_t voltage_mV = tele_value_16 * 10; // Convert to mV
    LOG_DEBUG("TELE_VINR: %u\n", tele_value_16);
    device->charging_mV = voltage_mV; // Store in device struct
    return device->charging_mV;
}

// Read TELE_VIN (When solar supply operation is detected,
// TELE_VIN will indicate 0)
// TODO(yaoyiheng): For testing purposes, TBD whether this is needed for flight.
uint16_t mppt_get_vin_voltage(mppt_t *device)
{
    if (!device->i2c) // Mock device
    {
        return device->VIN_mV;
    }
    uint8_t result_2_bytes[2];
    // Voltage values are read in 100*V (so 10mV increments)
    uint16_t tele_value_16 =
        mppt_send_instruction_and_read_2_byte(device, LT8491_TELE_VIN);
    uint16_t voltage_mV = tele_value_16 * 10; // Convert to mV
    LOG_DEBUG("TELE_VIN: %u\n", tele_value_16);
    device->VIN_mV = voltage_mV; // Store in device struct
    return device->VIN_mV;
}

// Read TELE_IIN (Input Current - from solar panels)
uint16_t mppt_get_current(mppt_t *device)
{
    if (!device->i2c)
    {
        return device->charging_mA; // Mock device
    }
    // Current values are read in mA directly
    uint16_t tele_value_16 =
        mppt_send_instruction_and_read_2_byte(device, LT8491_TELE_IIN);
    LOG_DEBUG("TELE_IIN: %u\n", tele_value_16);
    device->charging_mA = tele_value_16; // Store in device struct
    return device->charging_mA;
}
