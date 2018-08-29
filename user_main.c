/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "esp/uart.h"
#include "esp/gpio.h"

#include "espressif/esp_system.h"
#include "esp_spiffs.h"
#include "espressif/esp_common.h"

#include "spiffs.h"
#include <fcntl.h>
#include <math.h>

#include "secrets.h"

SemaphoreHandle_t temp_mutex;
uint16_t current_temp = 0;

#define GPIO_NUM_4 4

#define OVERSAMPLE 8

static void read_temp() 
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

static void display_temp()
{
    while(1)
    {
        if(xSemaphoreTake(temp_mutex, portMAX_DELAY))
        {
            printf("Current Temp: %d\n", current_temp);
            xSemaphoreGive(temp_mutex);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

static void check_temp()
{
    volatile uint8_t changed = 0;
    volatile uint8_t state = 0;

    gpio_enable(GPIO_NUM_4, GPIO_OUTPUT);

    while(1)
    {
        changed = 0;
        if(xSemaphoreTake(temp_mutex, portMAX_DELAY))
        {
            if(current_temp > 87)
            {
                if(state == 0)
                {
                    changed = 1;
                }
                state = 1;
            }
            else
            {
                if(state == 1)
                {
                    changed = 1;
                }
                state = 0;
            }
        }
        xSemaphoreGive(temp_mutex);

        if(changed)
        {
            gpio_write(GPIO_NUM_4, state);
            vTaskDelay((10*60) * (1000 / portTICK_PERIOD_MS));
        }
        else
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}

void setup_sta()
{
    struct sdk_station_config config = {
        .ssid = SSID,
        .password = PASSWORD,
    };

    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);
    sdk_wifi_station_connect();

    printf("setup_sta done");
}



/*****************************************************************************
 * FunctionName : app_main
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
//void app_main(void)
void user_init(void)
{
    uart_set_baud(0, 115200);
    printf("SDK version:%s\n", sdk_system_get_sdk_version());

    temp_mutex = xSemaphoreCreateMutex();
    /*
    esp_spiffs_init();
    printf("\nesp_spiffs_mount = %d\n", esp_spiffs_mount());
    
    printf("Free Heap: %d\n", sdk_system_get_free_heap_size());
    setup_sta();
    printf("Free Heap: %d\n", sdk_system_get_free_heap_size());
    */
    xTaskCreate(read_temp, "adc", 512, NULL, 10, NULL);
    xTaskCreate(display_temp, "display", 512, NULL, 10, NULL);
    xTaskCreate(check_temp, "check", 512, NULL, 10, NULL);

    printf("Free Heap: %d\n", sdk_system_get_free_heap_size());
}
