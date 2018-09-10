#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "espressif/esp_system.h"
#include "esp/gpio.h"
#include "esp/spi.h"

#include "kegerator.h"

#include "ucg.h"
#include "port.h"

#define GPIO_NUM_4 16 
#define SPI_BUS 1

volatile static uint8_t running = 0;

void read_temp() 
{
    uint32_t temp_debounce = 0;
    uint16_t temp;
    const double A = 0.001137155;
    const double B = 2.3259496E-4;
    const double C = 9.540003E-8;

    while(1)
    {
        temp = sdk_system_adc_read();

        if(temp_debounce == 0) 
        {
            temp_debounce = temp;
        }
        else
        {
            temp_debounce = temp_debounce - (temp_debounce / OVERSAMPLE) + (temp / OVERSAMPLE);
        }

        double Rth = 1000 * (1023 / (double)temp_debounce - 1);

        double temperature = (1 / (A + B * log(Rth) + C * pow(log(Rth), 3))) - 273.15;
        temperature = temperature * 1.8 + 32; // degrees F

        if(xSemaphoreTake(temp_mutex, portMAX_DELAY))
        {
            current_temp = (uint16_t)temperature;
            xSemaphoreGive(temp_mutex);
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void display_temp()
{
    ucg_t ucg;
    ucg.pin_list[UCG_PIN_CS] = 4;
    ucg.pin_list[UCG_PIN_CD] = 5;

    spi_init(SPI_BUS, SPI_MODE0, SPI_FREQ_DIV_80M, true, SPI_LITTLE_ENDIAN, true);
    ucg_Init(&ucg, ucg_dev_ssd1331_18x96x64_univision, ucg_ext_ssd1331_18, ucg_com_cb_esp8266_spi_hw);

    ucg_SetFontMode(&ucg, UCG_FONT_MODE_TRANSPARENT);
    ucg_SetFontPosTop(&ucg);

    while(1)
    {
        char temp[3];
        int len;

        if(xSemaphoreTake(temp_mutex, portMAX_DELAY))
        {
            //printf("Current Temp: %d\n", current_temp);
            snprintf(temp, 3, "%d", current_temp);
            xSemaphoreGive(temp_mutex);
        }

        if(setpoint_dirty)
        {
            write_config();
        }

        ucg_ClearScreen(&ucg);

        ucg_SetFont(&ucg, ucg_font_logisoso42_tr);
        ucg_SetColor(&ucg, 0, 255, 0, 0);
        len = ucg_DrawString(&ucg, 0, 5, 0, temp);

        snprintf(temp, 3, "%d", setpoint);
        ucg_SetFont(&ucg, ucg_font_logisoso32_tr);
        ucg_SetColor(&ucg, 0, 0, 255, 0);
        ucg_DrawString(&ucg, len + 3, 15, 0, temp);

        ucg_SetFont(&ucg, ucg_font_helvR08_tr);
        ucg_SetColor(&ucg, 0, 255, 255, 255);
        if(running)
        {
            ucg_DrawString(&ucg, 0, 55, 0, "* Power ON");
        } else {
            ucg_DrawString(&ucg, 0, 55, 0, "* Power OFF");
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void check_temp()
{
    volatile uint8_t changed = 0;

    gpio_enable(GPIO_NUM_4, GPIO_OUTPUT);

    while(1)
    {
        changed = 0;
        if(xSemaphoreTake(temp_mutex, portMAX_DELAY))
        {
            if(current_temp > setpoint)
            {
                if(running == 0)
                {
                    changed = 1;
                }
                running = 1;
            }
            else
            {
                if(running == 1)
                {
                    changed = 1;
                }
                running = 0;
            }
        }
        xSemaphoreGive(temp_mutex);

        if(changed)
        {
            gpio_write(GPIO_NUM_4, running);
            vTaskDelay((5/**60*/) * (1000 / portTICK_PERIOD_MS));
        }
        else
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}
