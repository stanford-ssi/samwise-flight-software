#include "mppt.h"

// Configuration Register Data (mirrors Python CFG tuple)
const lt8491_cfg_register_t CFG[9] = {
    {"CFG_RSENSE1", 0x28, 0x2710}, {"CFG_RIMON_OUT", 0x2A, 0x0BC2},
    {"CFG_RSENSE2", 0x2C, 0x0CE4}, {"CFG_RDACO", 0x2E, 0x3854},
    {"CFG_RFBOT1", 0x30, 0x0604},  {"CFG_RFBOUT2", 0x32, 0x0942},
    {"CFG_RDACI", 0x34, 0x0728},   {"RFBIN2", 0x36, 0x02DC},
    {"RDBIN1", 0x38, 0x03B9}};

// Telemetry Register Addresses (mirrors Python TELE tuple)
// Not directly used in the provided main loop logic in the same way as CFG,
// but defined for completeness or future use.
const uint8_t TELE[5] = {
    0x00, // TELE_TBAT
    0x02, // TELE_POUT
    0x04, // TELE_PIN
    0x08, // TELE_IOUT
    0x0A  // TELE_IIN
};

mppt_t mppt_mk(i2c_inst_t *i2c, uint8_t address)
{
    mppt_t device;
    device.i2c = i2c;
    device.address = address;
    device.is_charging = false;
    device.is_initialized = false;
    device.voltage = 0;
    device.current = 0;
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

    printf("Configuring LT8491 registers...\n");
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
        i2c_write_data(LT8491_I2C_ADDR, cmd_buf, 2);

        cmd_buf[0] = CFG[i].addr + 1;            // Register address for MSB
        cmd_buf[1] = (CFG[i].value >> 8) & 0xFF; // High byte
        i2c_write_data(LT8491_I2C_ADDR, cmd_buf, 2);

        // Read back the 16-bit value (LSB from CFG[i].addr, MSB from
        // CFG[i].addr+1)
        read_cmd = CFG[i].addr;
        i2c_write_then_read(LT8491_I2C_ADDR, &read_cmd, 1, read_buf_word, 2);
        read_value_check = (uint16_t)read_buf_word[0] |
                           ((uint16_t)read_buf_word[1] << 8); // LSB first

        LOG_INFO("%s : Set 0x%04X, Read 0x%04X (LSB:0x%02X MSB:0x%02X)\n",
                 CFG[i].name, CFG[i].value, read_value_check, read_buf_word[0],
                 read_buf_word[1]);
    }

    // Turn ON charging after configuration
    // Python: i2c.writeto_then_readfrom(LT8491_ADDR, bytearray([0x23, 1]),
    // result)
    cmd_buf[0] = LT8491_CTRL_CHRG_EN; // CTRL_CHRG_EN register
    cmd_buf[1] = 1;                   // Value to enable
    i2c_write_data(LT8491_I2C_ADDR, cmd_buf, 2);

    // Device completed initialization
    device->is_initialized = true;
}

uint16_t mppt_get_voltage(mppt_t *device)
{
    uint8_t result_2_bytes[2];
    uint8_t reg_to_read = 0x10; // TELE_VINR (Input Voltage)
    i2c_write_then_read(LT8491_I2C_ADDR, &reg_to_read, 1, result_2_bytes, 2);
    uint16_t tele_value_16 =
        (uint16_t)result_2_bytes[0] |
        ((uint16_t)result_2_bytes[1] << 8);          // Read in 10 mV increments
    float voltage_mV = (float)tele_value_16 / 10.0f; // Convert to mV
    LOG_DEBUG("TELE_IIN: %.2f mV\n", voltage_mV);
    device->voltage = (uint16_t)voltage_mV; // Store in device struct
    return device->voltage;
}

uint16_t mppt_get_current(mppt_t *device)
{
    uint8_t result_2_bytes[2];
    uint8_t reg_to_read = 0x0A; // TELE_IIN (Input Current)
    i2c_write_then_read(LT8491_I2C_ADDR, &reg_to_read, 1, result_2_bytes, 2);
    uint16_t tele_value_16 = (uint16_t)result_2_bytes[0] |
                             ((uint16_t)result_2_bytes[1] << 8); // Read in mA
    LOG_DEBUG("TELE_IIN: %u mA\n", tele_value_16);
    device->current = tele_value_16; // Store in device struct
    return device->current;
}
