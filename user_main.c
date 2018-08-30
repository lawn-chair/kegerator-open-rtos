/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
//#include "esp_spiffs.h"

//#include "spiffs.h"
//#include <fcntl.h>

#include "secrets.h"
#include "kegerator.h"
#include "control.h"
#include "httpd.h"
#include "json.h"

SemaphoreHandle_t temp_mutex;
uint16_t current_temp = 0;
uint16_t setpoint = MAX_TEMP;

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
    setup_sta();

    espFsInit((void*)(_binary_build_web_espfs_bin_start));
    httpdInit(builtInUrls, 80);

    xTaskCreate(read_temp, "adc", 512, NULL, 10, NULL);
    xTaskCreate(display_temp, "display", 512, NULL, 10, NULL);
    xTaskCreate(check_temp, "check", 512, NULL, 10, NULL);

    printf("Free Heap: %d\n", sdk_system_get_free_heap_size());
}
