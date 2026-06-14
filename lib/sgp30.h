#ifndef __SGP30_H__
#define __SGP30_H__

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// 引脚配置（用户可覆盖）
// 默认使用 I2C0 的标准引脚：GP4=SDA, GP5=SCL
// 在包含此头文件之前 #define 即可自定义
// ============================================================

#ifndef SGP30_SCL_PIN
#define SGP30_SCL_PIN   5
#endif

#ifndef SGP30_SDA_PIN
#define SGP30_SDA_PIN   4
#endif

// I2C 延时参数（单位：微秒），标准模式 ≈100kHz
#ifndef SGP30_I2C_DELAY_US
#define SGP30_I2C_DELAY_US   5
#endif

// ACK 等待超时（轮询次数上限）
#ifndef SGP30_ACK_TIMEOUT
#define SGP30_ACK_TIMEOUT    250
#endif

// ============================================================
// SGP30 I2C 地址（7位地址 0x58）
// ============================================================

#define SGP30_I2C_ADDR        0x58
#define SGP30_ADDR_WRITE      ((SGP30_I2C_ADDR << 1) | 0x00)   // 0xB0
#define SGP30_ADDR_READ       ((SGP30_I2C_ADDR << 1) | 0x01)   // 0xB1

// ============================================================
// SGP30 命令码
// ============================================================

#define SGP30_CMD_INIT_AIR_QUALITY      0x2003
#define SGP30_CMD_MEASURE_AIR_QUALITY   0x2008
#define SGP30_CMD_GET_BASELINE          0x2015
#define SGP30_CMD_SET_BASELINE          0x201E
#define SGP30_CMD_SET_HUMIDITY          0x2061
#define SGP30_CMD_MEASURE_TEST          0x2032
#define SGP30_CMD_GET_FEATURE_SET       0x202F
#define SGP30_CMD_MEASURE_RAW           0x2050
#define SGP30_CMD_GET_TVOC_INCEPTIVE    0x20B3
#define SGP30_CMD_SOFT_RESET            0x0006

// ============================================================
// 返回值定义
// ============================================================

#define SGP30_OK              0
#define SGP30_ERR_ACK          1
#define SGP30_ERR_CRC          2
#define SGP30_ERR_TIMEOUT      3

// ============================================================
// 数据结构
// ============================================================

typedef struct {
    uint16_t co2;       // eCO2 浓度 (ppm)
    uint16_t tvoc;      // TVOC 浓度 (ppb)
    uint8_t  crc_ok;    // CRC 校验是否通过
} sgp30_data_t;

// ============================================================
// 公开 API
// ============================================================

// 初始化 SGP30（GPIO 初始化 + 发送 init_air_quality 命令）
void SGP30_Init(void);

// 发送 2 字节命令到 SGP30
// 返回 SGP30_OK 或 SGP30_ERR_xxx
uint8_t SGP30_Write(uint8_t cmd_high, uint8_t cmd_low);

// 读取 SGP30 的 6 字节测量结果，解析为 sgp30_data_t
// 返回 SGP30_OK 或 SGP30_ERR_xxx
uint8_t SGP30_Read(sgp30_data_t *data);

// 设置环境湿度补偿（绝对湿度，单位 mg/m³）
uint8_t SGP30_SetHumidity(uint16_t abs_humidity);

// 获取/设置基线值（用于快速启动）
uint8_t SGP30_GetBaseline(uint16_t *eco2_baseline, uint16_t *tvoc_baseline);
uint8_t SGP30_SetBaseline(uint16_t eco2_baseline, uint16_t tvoc_baseline);

// 软复位
uint8_t SGP30_SoftReset(void);

// 测量测试（返回 OK 表示传感器通信正常）
uint8_t SGP30_MeasureTest(uint16_t *result);

// 获取特性集版本
uint8_t SGP30_GetFeatureSet(uint16_t *version);

// 获取原始测量值
uint8_t SGP30_MeasureRaw(uint16_t *ethanol, uint16_t *h2);

// CRC-8 校验工具函数
uint8_t SGP30_CRC8(const uint8_t *data, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* __SGP30_H__ */
