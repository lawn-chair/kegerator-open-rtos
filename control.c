#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"


#include "esp/gpio.h"

#include "kegerator.h"

#define GPIO_NUM_4 4

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

void check_temp()
{
    volatile uint8_t changed = 0;
    volatile uint8_t state = 0;

    gpio_enable(GPIO_NUM_4, GPIO_OUTPUT);

    while(1)
    {
        changed = 0;
        if(xSemaphoreTake(temp_mutex, portMAX_DELAY))
        {
            if(current_temp > setpoint)
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
