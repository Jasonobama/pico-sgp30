# pico-sgp30 — Air Quality Monitor for Raspberry Pi Pico 2

基于 Raspberry Pi Pico 2 (RP2350) 的空气质量监测器，使用 SGP30 传感器检测 eCO2 和 TVOC 浓度，通过 SSD1306 I2C OLED 实时显示。

本项目使用 **Raspberry Pi Pico 2 C/C++ SDK** 工具包编写，基于 CMake + ARM GNU Toolchain 构建系统。

## 硬件需求

| 器件 | 型号 | 数量 |
|------|------|------|
| 主控 | Raspberry Pi Pico 2 (RP2350) | 1 |
| 传感器 | SGP30 (I2C 地址 0x58) | 1 |
| 显示屏 | SSD1306 128×64 I2C OLED (地址 0x3C) | 1 |

## 接线

| Pico 2 引脚 | 连接设备 | 说明 |
|-------------|----------|------|
| GPIO4 | SGP30 SDA | 软件 I2C |
| GPIO5 | SGP30 SCL | 软件 I2C |
| GPIO8 | SSD1306 SDA | 硬件 I2C0 |
| GPIO9 | SSD1306 SCL | 硬件 I2C0 |
| 3V3(OUT) | VCC (两设备) | 供电 |
| GND | GND (两设备) | 共地 |

> 如需修改 SGP30 引脚，在 `#include "sgp30.h"` 之前 `#define SGP30_SDA_PIN x` / `#define SGP30_SCL_PIN x`。
> 如需修改 SSD1306 引脚，在 `#include "ssd1306.h"` 之前 `#define SSD1306_SDA_PIN x` / `#define SSD1306_SCL_PIN x`。

## 目录结构

```
pico-sgp30/
├── CMakeLists.txt          # CMake 构建配置
├── pico-sgp30.c            # 主程序（业务逻辑）
├── lib/
│   ├── sgp30.h             # SGP30 驱动头文件
│   ├── sgp30.c             # SGP30 驱动实现（软件 I2C）
│   ├── ssd1306.h           # SSD1306 驱动头文件
│   └── ssd1306.c           # SSD1306 驱动实现（硬件 I2C + 5×8 字体）
|
└── README.md               # README.md
```

## 库文件 API

### SGP30 (`lib/sgp30.h`)

采用软件 I2C（bit-bang）实现，无需占用硬件 I2C 外设。

| 函数 | 说明 | 返回值 |
|------|------|--------|
| `void SGP30_Init(void)` | 初始化 GPIO 并发送 `init_air_quality` 命令 | — |
| `uint8_t SGP30_Write(uint8_t hi, uint8_t lo)` | 向 SGP30 发送 2 字节命令 | `SGP30_OK` / `SGP30_ERR_ACK` |
| `uint8_t SGP30_Read(sgp30_data_t *data)` | 读取 6 字节测量结果，解析 CO2/TVOC/CRC | `SGP30_OK` / `SGP30_ERR_ACK` |
| `uint8_t SGP30_SetHumidity(uint16_t abs_humidity)` | 设置绝对湿度补偿 (mg/m³) | 同上 |
| `uint8_t SGP30_GetBaseline(uint16_t *eco2, uint16_t *tvoc)` | 读取基线值（快速启动用） | 同上 |
| `uint8_t SGP30_SetBaseline(uint16_t eco2, uint16_t tvoc)` | 写入基线值 | 同上 |
| `uint8_t SGP30_SoftReset(void)` | 软复位传感器 | 同上 |
| `uint8_t SGP30_MeasureTest(uint16_t *result)` | 自检测试（返回 0xD400 为正常） | 同上 |
| `uint8_t SGP30_GetFeatureSet(uint16_t *version)` | 获取特性集版本号 | 同上 |
| `uint8_t SGP30_MeasureRaw(uint16_t *ethanol, uint16_t *h2)` | 读取原始乙醇/H₂信号值 | 同上 |
| `uint8_t SGP30_CRC8(const uint8_t *data, uint8_t len)` | CRC-8 校验工具 (多项式 0x31) | CRC 值 |

数据结构 `sgp30_data_t`：

```c
typedef struct {
    uint16_t co2;    // eCO2 浓度 (ppm)
    uint16_t tvoc;   // TVOC 浓度 (ppb)
    uint8_t  crc_ok; // CRC 校验是否通过
} sgp30_data_t;
```

可覆盖宏：
- `SGP30_SDA_PIN` — 默认 4
- `SGP30_SCL_PIN` — 默认 5
- `SGP30_I2C_DELAY_US` — 默认 5 (≈100kHz)
- `SGP30_ACK_TIMEOUT` — 默认 250

### SSD1306 (`lib/ssd1306.h`)

采用硬件 I2C 实现，内置 5×8 ASCII 全字符集 (96 个字形)，直接写屏架构。

| 函数 | 说明 |
|------|------|
| `void ssd1306_init(void)` | 初始化 I2C 外设和 OLED 控制器，最后清屏 |
| `void ssd1306_clear(void)` | 清空全屏（8 个 page 逐页清零） |
| `void ssd1306_draw_string(uint8_t page, uint8_t col, const char *str)` | 在指定 page(0-7) 和 col(0-127) 绘制字符串，自动用空格填充至行尾 |

可覆盖宏：
- `SSD1306_I2C_PORT` — 默认 `i2c0`
- `SSD1306_SDA_PIN` — 默认 8
- `SSD1306_SCL_PIN` — 默认 9
- `SSD1306_I2C_ADDR` — 默认 `0x3C`
- `SSD1306_WIDTH` — 默认 128
- `SSD1306_HEIGHT` — 默认 64

## 主程序流程 (`pico-sgp30.c`)

```
main()
├── stdio_init_all()           // 初始化 USB 串口
├── ssd1306_init()              // 初始化 OLED
│   ├── i2c_init(i2c0, 400kHz)
│   ├── gpio_set_function(SDA/SCL)
│   ├── 发送 OLED 初始化命令序列
│   └── ssd1306_clear()
├── SGP30_Init()                // 初始化 SGP30 (软件 I2C GPIO4/5)
│   └── 发送 init_air_quality 命令
│
└── while(1) 主循环:
    ├── 发送 measure_air_quality 命令 (0x2008)
    ├── 读取 6 字节数据 + CRC 校验
    ├── [预热期 15s] 显示倒计时
    └── [正常运行] 显示 eCO2 / TVOC / CRC 状态
        sleep_ms(2000)
```

### OLED 显示布局

```
┌──────────────────────────┐
│ Page 0:  SGP30 Sensor     │  ← 标题行（仅启动时写入一次）
│ Page 1:                   │
│ Page 2:  eCO2:   400 ppm  │  ← 二氧化碳浓度（5 位右对齐）
│ Page 3:                   │
│ Page 4:  TVOC:     0 ppb  │  ← 挥发性有机物浓度
│ Page 5:                   │
│ Page 6:  CRC FAIL!        │  ← CRC 错误提示（仅出错时显示）
│ Page 7:                   │
└──────────────────────────┘
```

## 构建与部署

### 前置条件

- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) 2.2.0+
- ARM GNU Toolchain (`arm-none-eabi-gcc` 14.2+)
- CMake 3.13+
- Ninja 构建系统
- VS Code + Raspberry Pi Pico 扩展（可选）

### 构建步骤

```bash
# 1. 配置 CMake
cmake -B build -G "Ninja" .

# 2. 编译
cmake --build build

# 3. 产物位于 build/pico-sgp30.uf2
```

### 烧录到 Pico 2

1. 按住 BOOTSEL 按钮，连接 USB
2. Pico 2 会显示为 USB 大容量存储设备（RPI-RP2）
3. 将 `build/pico-sgp30.uf2` 拖入该驱动器
4. Pico 2 自动重启并运行程序

### 查看串口输出

程序通过 USB CDC 输出调试信息（波特率自动协商）：

```bash
# Linux / macOS
screen /dev/ttyACM0 115200

# Windows (PuTTY 或其他终端)
# COM 端口号在设备管理器中查看
```

预期输出：

```
SGP30 + SSD1306 Air Quality Monitor
SGP30 initialized, waiting for warm-up (15s)...
Warming up... 14 s remaining
...
eCO2: 412 ppm, TVOC: 5 ppb
eCO2: 415 ppm, TVOC: 6 ppb
```

## 自定义配置

### 更换 I2C 总线

如果 SGP30 和 SSD1306 需要共用同一组 I2C 引脚（共 2 根线），可在 `pico-sgp30.c` 顶部添加：

```c
// 使 SGP30 与 SSD1306 共用 GPIO8/9
#define SGP30_SDA_PIN   8
#define SGP30_SCL_PIN   9

#include "sgp30.h"
#include "ssd1306.h"
```

### 调整测量间隔

修改 `pico-sgp30.c` 第 67 行的 `sleep_ms(2000)` 值（SGP30 最快约 1 秒/次）。

### 湿度补偿

在主循环中添加绝对湿度补偿可提高读数精度（推荐但非必需）：

```c
// 例如 25°C / 50%RH → 绝对湿度约 11.5 g/m³ → 11500 mg/m³
SGP30_SetHumidity(11500);
```

## 许可证

MIT License
