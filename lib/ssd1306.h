#ifndef __SSD1306_H__
#define __SSD1306_H__

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// 引脚配置（用户可覆盖）
// 默认使用 I2C0：GP8=SDA, GP9=SCL
// ============================================================

#ifndef SSD1306_I2C_PORT
#define SSD1306_I2C_PORT    i2c0
#endif

#ifndef SSD1306_SDA_PIN
#define SSD1306_SDA_PIN     8
#endif

#ifndef SSD1306_SCL_PIN
#define SSD1306_SCL_PIN     9
#endif

#ifndef SSD1306_I2C_ADDR
#define SSD1306_I2C_ADDR    0x3C
#endif

#ifndef SSD1306_WIDTH
#define SSD1306_WIDTH       128
#endif

#ifndef SSD1306_HEIGHT
#define SSD1306_HEIGHT      64
#endif

#ifndef SSD1306_PAGES
#define SSD1306_PAGES       8
#endif

// ============================================================
// 公开 API
// ============================================================

void ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_draw_string(uint8_t page, uint8_t col, const char *str);

#ifdef __cplusplus
}
#endif

#endif /* __SSD1306_H__ */
