#include "sgp30.h"

// ============================================================
// GPIO 操作宏（硬件抽象层）
// ============================================================

#define SCL_HIGH()      gpio_put(SGP30_SCL_PIN, 1)
#define SCL_LOW()       gpio_put(SGP30_SCL_PIN, 0)
#define SDA_HIGH()      gpio_put(SGP30_SDA_PIN, 1)
#define SDA_LOW()       gpio_put(SGP30_SDA_PIN, 0)
#define SDA_READ()      gpio_get(SGP30_SDA_PIN)
#define SDA_OUT()       gpio_set_dir(SGP30_SDA_PIN, GPIO_OUT)
#define SDA_IN()        gpio_set_dir(SGP30_SDA_PIN, GPIO_IN)
#define DELAY_US(n)     sleep_us(n)

// ============================================================
// GPIO 初始化
// ============================================================

static void sgp30_gpio_init(void)
{
    gpio_init(SGP30_SCL_PIN);
    gpio_init(SGP30_SDA_PIN);
    gpio_set_dir(SGP30_SCL_PIN, GPIO_OUT);
    gpio_set_dir(SGP30_SDA_PIN, GPIO_OUT);
    gpio_put(SGP30_SCL_PIN, 1);
    gpio_put(SGP30_SDA_PIN, 1);
}

// ============================================================
// 软件 I2C 底层函数
// ============================================================

static void i2c_start(void)
{
    SDA_OUT();
    SDA_HIGH();
    SCL_HIGH();
    DELAY_US(SGP30_I2C_DELAY_US);
    SDA_LOW();
    DELAY_US(SGP30_I2C_DELAY_US);
    SCL_LOW();
    DELAY_US(SGP30_I2C_DELAY_US);
}

static void i2c_stop(void)
{
    SDA_OUT();
    SDA_LOW();
    SCL_LOW();
    DELAY_US(SGP30_I2C_DELAY_US);
    SCL_HIGH();
    DELAY_US(SGP30_I2C_DELAY_US);
    SDA_HIGH();
    DELAY_US(SGP30_I2C_DELAY_US);
}

static uint8_t i2c_wait_ack(void)
{
    uint16_t timeout = 0;
    SDA_IN();
    SDA_HIGH();
    DELAY_US(1);
    SCL_HIGH();
    DELAY_US(1);
    while (SDA_READ())
    {
        timeout++;
        if (timeout > SGP30_ACK_TIMEOUT)
        {
            i2c_stop();
            return SGP30_ERR_ACK;
        }
        sleep_us(1);
    }
    SCL_LOW();
    return SGP30_OK;
}

static void i2c_send_ack(void)
{
    SCL_LOW();
    SDA_OUT();
    SDA_LOW();
    DELAY_US(SGP30_I2C_DELAY_US);
    SCL_HIGH();
    DELAY_US(SGP30_I2C_DELAY_US);
    SCL_LOW();
}

static void i2c_send_nack(void)
{
    SCL_LOW();
    SDA_OUT();
    SDA_HIGH();
    DELAY_US(SGP30_I2C_DELAY_US);
    SCL_HIGH();
    DELAY_US(SGP30_I2C_DELAY_US);
    SCL_LOW();
}

static void i2c_write_byte(uint8_t data)
{
    uint8_t i;
    SDA_OUT();
    SCL_LOW();
    for (i = 0; i < 8; i++)
    {
        if (data & 0x80)
            SDA_HIGH();
        else
            SDA_LOW();
        data <<= 1;
        DELAY_US(SGP30_I2C_DELAY_US);
        SCL_HIGH();
        DELAY_US(SGP30_I2C_DELAY_US);
        SCL_LOW();
        DELAY_US(SGP30_I2C_DELAY_US);
    }
}

static uint8_t i2c_read_byte(uint8_t send_ack)
{
    uint8_t i, value = 0;
    SDA_IN();
    for (i = 0; i < 8; i++)
    {
        SCL_LOW();
        DELAY_US(SGP30_I2C_DELAY_US);
        SCL_HIGH();
        value <<= 1;
        if (SDA_READ())
            value |= 0x01;
        DELAY_US(SGP30_I2C_DELAY_US);
    }
    if (send_ack)
        i2c_send_ack();
    else
        i2c_send_nack();
    return value;
}

// ============================================================
// CRC-8 校验（多项式 0x31, 初始值 0xFF）
// 参考 SGP30 数据手册
// ============================================================

uint8_t SGP30_CRC8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;
    uint8_t i, j;
    for (i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x80)
                crc = (uint8_t)((crc << 1) ^ 0x31);
            else
                crc <<= 1;
        }
    }
    return crc;
}

// ============================================================
// SGP30 公开 API
// ============================================================

void SGP30_Init(void)
{
    sgp30_gpio_init();
    SGP30_Write((SGP30_CMD_INIT_AIR_QUALITY >> 8) & 0xFF,
                 SGP30_CMD_INIT_AIR_QUALITY & 0xFF);
}

uint8_t SGP30_Write(uint8_t cmd_high, uint8_t cmd_low)
{
    uint8_t ret;
    i2c_start();
    i2c_write_byte(SGP30_ADDR_WRITE);
    ret = i2c_wait_ack();
    if (ret != SGP30_OK) return ret;
    i2c_write_byte(cmd_high);
    ret = i2c_wait_ack();
    if (ret != SGP30_OK) return ret;
    i2c_write_byte(cmd_low);
    ret = i2c_wait_ack();
    if (ret != SGP30_OK) return ret;
    i2c_stop();
    sleep_ms(10);
    return SGP30_OK;
}

uint8_t SGP30_Read(sgp30_data_t *data)
{
    uint8_t buf[6];
    uint8_t ret, i;

    i2c_start();
    i2c_write_byte(SGP30_ADDR_READ);
    ret = i2c_wait_ack();
    if (ret != SGP30_OK) return ret;

    for (i = 0; i < 5; i++)
        buf[i] = i2c_read_byte(1);   // 前 5 字节回复 ACK
    buf[5] = i2c_read_byte(0);       // 最后一字节回复 NACK

    i2c_stop();

    data->crc_ok = 1;
    if (SGP30_CRC8(buf, 2) != buf[2])   data->crc_ok = 0;
    if (SGP30_CRC8(buf + 3, 2) != buf[5]) data->crc_ok = 0;

    data->co2  = ((uint16_t)buf[0] << 8) | buf[1];
    data->tvoc = ((uint16_t)buf[3] << 8) | buf[4];

    return SGP30_OK;
}

uint8_t SGP30_SetHumidity(uint16_t abs_humidity)
{
    uint8_t cmd_high = (SGP30_CMD_SET_HUMIDITY >> 8) & 0xFF;
    uint8_t cmd_low  = SGP30_CMD_SET_HUMIDITY & 0xFF;
    uint8_t data_high = (abs_humidity >> 8) & 0xFF;
    uint8_t data_low  = abs_humidity & 0xFF;
    uint8_t crc_val   = SGP30_CRC8(&data_low, 1);
    uint8_t ret;

    i2c_start();
    i2c_write_byte(SGP30_ADDR_WRITE);
    ret = i2c_wait_ack();  if (ret != SGP30_OK) return ret;
    i2c_write_byte(cmd_high);
    ret = i2c_wait_ack();  if (ret != SGP30_OK) return ret;
    i2c_write_byte(cmd_low);
    ret = i2c_wait_ack();  if (ret != SGP30_OK) return ret;
    i2c_write_byte(data_high);
    ret = i2c_wait_ack();  if (ret != SGP30_OK) return ret;
    i2c_write_byte(data_low);
    ret = i2c_wait_ack();  if (ret != SGP30_OK) return ret;
    i2c_write_byte(crc_val);
    ret = i2c_wait_ack();  if (ret != SGP30_OK) return ret;
    i2c_stop();
    sleep_ms(10);
    return SGP30_OK;
}

uint8_t SGP30_GetBaseline(uint16_t *eco2_baseline, uint16_t *tvoc_baseline)
{
    uint8_t buf[6];
    uint8_t ret, i;

    ret = SGP30_Write((SGP30_CMD_GET_BASELINE >> 8) & 0xFF,
                       SGP30_CMD_GET_BASELINE & 0xFF);
    if (ret != SGP30_OK) return ret;
    sleep_ms(10);

    i2c_start();
    i2c_write_byte(SGP30_ADDR_READ);
    ret = i2c_wait_ack();  if (ret != SGP30_OK) return ret;
    for (i = 0; i < 5; i++)
        buf[i] = i2c_read_byte(1);
    buf[5] = i2c_read_byte(0);
    i2c_stop();

    if (SGP30_CRC8(buf, 2) != buf[2])     return SGP30_ERR_CRC;
    if (SGP30_CRC8(buf + 3, 2) != buf[5]) return SGP30_ERR_CRC;

    *eco2_baseline  = ((uint16_t)buf[0] << 8) | buf[1];
    *tvoc_baseline  = ((uint16_t)buf[3] << 8) | buf[4];
    return SGP30_OK;
}

uint8_t SGP30_SetBaseline(uint16_t eco2_baseline, uint16_t tvoc_baseline)
{
    uint8_t cmd_high = (SGP30_CMD_SET_BASELINE >> 8) & 0xFF;
    uint8_t cmd_low  = SGP30_CMD_SET_BASELINE & 0xFF;
    uint8_t eco2_h   = (eco2_baseline >> 8) & 0xFF;
    uint8_t eco2_l   = eco2_baseline & 0xFF;
    uint8_t tvoc_h   = (tvoc_baseline >> 8) & 0xFF;
    uint8_t tvoc_l   = tvoc_baseline & 0xFF;
    uint8_t crc_eco2 = SGP30_CRC8(&eco2_l, 1);
    uint8_t crc_tvoc = SGP30_CRC8(&tvoc_l, 1);
    uint8_t ret;

    i2c_start();
    i2c_write_byte(SGP30_ADDR_WRITE);
    ret = i2c_wait_ack();  if (ret != SGP30_OK) return ret;
    i2c_write_byte(cmd_high);
    ret = i2c_wait_ack();  if (ret != SGP30_OK) return ret;
    i2c_write_byte(cmd_low);
    ret = i2c_wait_ack();  if (ret != SGP30_OK) return ret;
    i2c_write_byte(eco2_h);
    ret = i2c_wait_ack();  if (ret != SGP30_OK) return ret;
    i2c_write_byte(eco2_l);
    ret = i2c_wait_ack();  if (ret != SGP30_OK) return ret;
    i2c_write_byte(crc_eco2);
    ret = i2c_wait_ack();  if (ret != SGP30_OK) return ret;
    i2c_write_byte(tvoc_h);
    ret = i2c_wait_ack();  if (ret != SGP30_OK) return ret;
    i2c_write_byte(tvoc_l);
    ret = i2c_wait_ack();  if (ret != SGP30_OK) return ret;
    i2c_write_byte(crc_tvoc);
    ret = i2c_wait_ack();  if (ret != SGP30_OK) return ret;
    i2c_stop();
    sleep_ms(10);
    return SGP30_OK;
}

uint8_t SGP30_SoftReset(void)
{
    uint8_t ret;
    i2c_start();
    i2c_write_byte(0x00);
    ret = i2c_wait_ack();
    if (ret == SGP30_OK)
    {
        i2c_write_byte(0x06);
        ret = i2c_wait_ack();
    }
    i2c_stop();
    sleep_ms(10);
    return ret;
}

uint8_t SGP30_MeasureTest(uint16_t *result)
{
    uint8_t buf[3];
    uint8_t ret, i;

    ret = SGP30_Write((SGP30_CMD_MEASURE_TEST >> 8) & 0xFF,
                       SGP30_CMD_MEASURE_TEST & 0xFF);
    if (ret != SGP30_OK) return ret;
    sleep_ms(220);

    i2c_start();
    i2c_write_byte(SGP30_ADDR_READ);
    ret = i2c_wait_ack();  if (ret != SGP30_OK) return ret;
    for (i = 0; i < 2; i++)
        buf[i] = i2c_read_byte(1);
    buf[2] = i2c_read_byte(0);
    i2c_stop();

    if (SGP30_CRC8(buf, 2) != buf[2]) return SGP30_ERR_CRC;
    *result = ((uint16_t)buf[0] << 8) | buf[1];
    return SGP30_OK;
}

uint8_t SGP30_GetFeatureSet(uint16_t *version)
{
    uint8_t buf[3];
    uint8_t ret, i;

    ret = SGP30_Write((SGP30_CMD_GET_FEATURE_SET >> 8) & 0xFF,
                       SGP30_CMD_GET_FEATURE_SET & 0xFF);
    if (ret != SGP30_OK) return ret;
    sleep_ms(10);

    i2c_start();
    i2c_write_byte(SGP30_ADDR_READ);
    ret = i2c_wait_ack();  if (ret != SGP30_OK) return ret;
    for (i = 0; i < 2; i++)
        buf[i] = i2c_read_byte(1);
    buf[2] = i2c_read_byte(0);
    i2c_stop();

    if (SGP30_CRC8(buf, 2) != buf[2]) return SGP30_ERR_CRC;
    *version = ((uint16_t)buf[0] << 8) | buf[1];
    return SGP30_OK;
}

uint8_t SGP30_MeasureRaw(uint16_t *ethanol, uint16_t *h2)
{
    uint8_t buf[6];
    uint8_t ret, i;

    ret = SGP30_Write((SGP30_CMD_MEASURE_RAW >> 8) & 0xFF,
                       SGP30_CMD_MEASURE_RAW & 0xFF);
    if (ret != SGP30_OK) return ret;
    sleep_ms(25);

    i2c_start();
    i2c_write_byte(SGP30_ADDR_READ);
    ret = i2c_wait_ack();  if (ret != SGP30_OK) return ret;
    for (i = 0; i < 5; i++)
        buf[i] = i2c_read_byte(1);
    buf[5] = i2c_read_byte(0);
    i2c_stop();

    if (SGP30_CRC8(buf, 2) != buf[2])     return SGP30_ERR_CRC;
    if (SGP30_CRC8(buf + 3, 2) != buf[5]) return SGP30_ERR_CRC;

    *ethanol = ((uint16_t)buf[0] << 8) | buf[1];
    *h2      = ((uint16_t)buf[3] << 8) | buf[4];
    return SGP30_OK;
}
