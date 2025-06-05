/**
 * @author: Marc Aaron Reyes
 * @date: 2025-05-21
 */
#include "adm1176.h"

adm1176_t adm1176_mk_mock()
{
    return (adm1176_t){.i2c = NULL, .address = 0x00, .sense_resistor = 0.1f};
}

adm1176_t adm1176_mk(i2c_inst_t *i2c, uint8_t address, float sense_resistor)
{
    return (adm1176_t){
        .i2c = i2c, .address = address, .sense_resistor = sense_resistor};
}
bool adm1176_config(adm1176_t *pwm, int *mode, int mode_len)
{
    _cmd_buf[0] = 0x0;

    for (int i = 0; i < mode_len; i++)
    {
        switch (mode[i])
        {
            case 1:
                _cmd_buf[0] |= (1 << 0);
                break;
            case 2:
                _cmd_buf[0] |= (1 << 1);
                break;
            case 3:
                _cmd_buf[0] |= (1 << 2);
                break;
            case 4:
                _cmd_buf[0] |= (1 << 3);
                break;
            case 5:
                _cmd_buf[0] |= (1 << 4);
                break;
        }
    }

    if (i2c_write_blocking_until(pwm->i2c, pwm->address, _cmd_buf, 1, false,
                                 make_timeout_time_ms(I2C_TIMEOUT_MS)) != 1)
    {
        return false;
    }

    return true;
}

static inline void adm1176_print_status(adm1176_t *pwm)
{
    if (!pwm || !pwm->i2c)
    {
        LOG_DEBUG("ADM1176: NULL pointer\n");
        return;
    }
    uint8_t status;
    adm1176_read_status(pwm, &status);
    LOG_DEBUG("ADM Status %u\n", status);
}

float adm1176_get_voltage(adm1176_t *pwm)
{
    if (!pwm || !pwm->i2c)
    {
        LOG_DEBUG("ADM1176: NULL pointer\n");
        return 4.2f;
    }
    adm1176_on(pwm);
    sleep_ms(1);
    i2c_read_blocking_until(pwm->i2c, pwm->address, _read_buf, 3, false,
                            make_timeout_time_ms(I2C_TIMEOUT_MS));

    float raw_volts = ((_read_buf[0] << 8) | (_read_buf[2] & DATA_V_MASK)) >> 4;
    return (26.35f / 4096.0f) * raw_volts;
}

float adm1176_get_current(adm1176_t *pwm)
{
    if (!pwm || !pwm->i2c)
    {
        LOG_DEBUG("ADM1176: NULL pointer\n");
        return 1.0f;
    }
    adm1176_on(pwm);
    sleep_ms(1);
    i2c_read_blocking_until(pwm->i2c, pwm->address, _read_buf, 3, false,
                            make_timeout_time_ms(I2C_TIMEOUT_MS));

    float raw_amps = ((_read_buf[0] << 8) | (_read_buf[2] & DATA_V_MASK)) >> 4;
    return ((0.10584f / 4096.0f) * raw_amps) / pwm->sense_resistor;
}

void adm1176_on(adm1176_t *pwm)
{
    if (!pwm || !pwm->i2c)
    {
        LOG_DEBUG("ADM1176: NULL pointer\n");
        return;
    }
    _ext_cmd_buf[0] = 0x83;
    _ext_cmd_buf[1] = 0;
    i2c_write_blocking_until(pwm->i2c, pwm->address, _ext_cmd_buf, 2, false,
                             make_timeout_time_ms(I2C_TIMEOUT_MS));
    int modes[2] = {1, 3};
    adm1176_config(pwm, modes, 2);
    LOG_DEBUG("ADM Cmd: %X\n", _cmd_buf[0]);
}

void adm1176_off(adm1176_t *pwm)
{
    if (!pwm || !pwm->i2c)
    {
        LOG_DEBUG("ADM1176: NULL pointer\n");
        return;
    }
    _ext_cmd_buf[0] = 0x83;
    _ext_cmd_buf[1] = 1;
    i2c_write_blocking_until(pwm->i2c, pwm->address, _ext_cmd_buf, 2, false,
                             make_timeout_time_ms(I2C_TIMEOUT_MS));
}

// Read the 8-bit STATUS register from the ADM1176.
bool adm1176_read_status(adm1176_t *pwm, uint8_t *status_out)
{
    if (!pwm || !pwm->i2c)
    {
        LOG_DEBUG("ADM1176: NULL pointer\n");
        return true;
    }
    // 1) Write the command byte with STATUS_RD = 1 (bit 6).
    uint8_t cmd = (1 << 6); // C6 = 1 → STATUS_RD
    if (i2c_write_blocking_until(pwm->i2c, pwm->address, &cmd, 1, false,
                                 make_timeout_time_ms(I2C_TIMEOUT_MS)) != 1)
    {
        return false;
    }

    // 2) Read one byte back from the device.
    int ret =
        i2c_read_blocking_until(pwm->i2c, pwm->address, status_out, 1, false,
                                make_timeout_time_ms(I2C_TIMEOUT_MS));
    return (ret == 1);
}
