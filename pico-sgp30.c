#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "sgp30.h"
#include "ssd1306.h"

int main()
{
    char buf[32];
    sgp30_data_t data;
    uint8_t ret;
    bool warmed_up = false;

    stdio_init_all();

    printf("SGP30 + SSD1306 Air Quality Monitor\n");

    ssd1306_init();
    ssd1306_draw_string(0, 0, "SGP30 Sensor");
    ssd1306_draw_string(2, 0, "Initializing...");

    SGP30_Init();
    printf("SGP30 initialized, waiting for warm-up (15s)...\n");

    uint32_t start_ms = to_ms_since_boot(get_absolute_time());

    while (1) {
        uint32_t now = to_ms_since_boot(get_absolute_time());

        ret = SGP30_Write((SGP30_CMD_MEASURE_AIR_QUALITY >> 8) & 0xFF,
                           SGP30_CMD_MEASURE_AIR_QUALITY & 0xFF);
        if (ret != SGP30_OK) {
            ssd1306_draw_string(2, 0, "I2C Write Err!");
            sleep_ms(1000);
            continue;
        }

        ret = SGP30_Read(&data);
        if (ret != SGP30_OK) {
            ssd1306_draw_string(2, 0, "I2C Read Err! ");
            sleep_ms(1000);
            continue;
        }

        if (!warmed_up && (now - start_ms) < 15000 && data.co2 == 400 && data.tvoc == 0) {
            uint32_t elapsed = (now - start_ms) / 1000;
            snprintf(buf, sizeof(buf), "Warm-up %lus...  ", 15 - elapsed);
            ssd1306_draw_string(2, 0, buf);
            printf("Warming up... %lu s remaining\n", 15 - elapsed);
            sleep_ms(1000);
            continue;
        }

        warmed_up = true;

        snprintf(buf, sizeof(buf), "eCO2: %5u ppm", data.co2);
        ssd1306_draw_string(2, 0, buf);

        snprintf(buf, sizeof(buf), "TVOC: %5u ppb", data.tvoc);
        ssd1306_draw_string(4, 0, buf);

        ssd1306_draw_string(6, 0, data.crc_ok ? "" : "CRC FAIL!");

        printf("eCO2: %u ppm, TVOC: %u ppb%s\n",
               data.co2, data.tvoc, data.crc_ok ? "" : " [CRC FAIL]");

        sleep_ms(2000);
    }

    return 0;
}
