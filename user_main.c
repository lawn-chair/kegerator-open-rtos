/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "espressif/esp_common.h"

#include "libesphttpd/httpd.h"
#include "libesphttpd/espfs.h"
#include "libesphttpd/webpages-espfs.h"

#include "jsmn.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "esp/uart.h"

#include "espressif/esp_system.h"
#include "esp_spiffs.h"

#include "spiffs.h"

#include "secrets.h"
#include "kegerator.h"
#include "control.h"
#include "httpd.h"
#include "json.h"

SemaphoreHandle_t temp_mutex;
uint16_t current_temp = 0;
uint16_t setpoint = MAX_TEMP;
uint8_t setpoint_dirty = 0;

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

void load_config()
{
    char buf[32];

    printf("load_config()\n");
    FILE *f = fopen("/config.json", "r");
    if(!f)
    {
        f = fopen("/config.json", "w");
        sprintf(buf, "{\"setpoint\": %d}", MAX_TEMP);
        fwrite(buf, strlen(buf), 1, f);
        fclose(f);
    } else {
        fread(buf, 1, 32, f);
        fclose(f);
    }

    jsmntok_t t[3];
    jsmn_parser p;
    jsmn_init(&p);
    jsmn_parse(&p, buf, strlen(buf), t, sizeof(t)/sizeof(t[0]));

    if(jsoneq(buf, &t[1], "setpoint") == 0)
    {
        int32_t value = strntoi(buf + t[2].start, t[2].end - t[2].start);
        setpoint = value;
        setpoint_dirty = 0;
    }
}

void write_config()
{
    char buf[32];

    FILE *f = fopen("/config.json", "w");
    if(xSemaphoreTake(temp_mutex, portMAX_DELAY))
    {
        sprintf(buf, "{\"setpoint\": %d}", setpoint);
        fwrite(buf, strlen(buf), 1, f);
        setpoint_dirty = 0;
        xSemaphoreGive(temp_mutex);
    }
    fclose(f);
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

    esp_spiffs_init();
    printf("\nesp_spiffs_mount = %d\n", esp_spiffs_mount());

    load_config();
    
    setup_sta();

    espFsInit((void*)(_binary_build_web_espfs_bin_start));
    httpdInit(builtInUrls, 80);

    xTaskCreate(read_temp, "adc", 512, NULL, 10, NULL);
    xTaskCreate(display_temp, "display", 512, NULL, 10, NULL);
    xTaskCreate(check_temp, "check", 512, NULL, 10, NULL);

    printf("Free Heap: %d\n", sdk_system_get_free_heap_size());
}
