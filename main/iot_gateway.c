#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"
#include "nvs.h"
#include "wifi_station.h"
#include "blecent.h"
#include "cmd_iot_gateway.h"
#include "iot_gateway_main.h"

/* Global Definations*/


/* Global Varibal */
static const char * TAG = "iot-gateway.c";


void app_main(void) {
    ESP_LOGI(TAG, "iot-gateway for Sesame5 [2025/3/18][002]");
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGI(TAG, "initial nvs flash failed, erase then init again!");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_LOGI(TAG, "initial nvs status=%d!", ret);
    
    //Init Console for command line
    ESP_LOGI(TAG, "initial console");
    iot_gateway_console_init();
    
    //Init WiFi Station mode
    ESP_LOGI(TAG, "initial wifi station!");
    wifi_init_sta();

    //Init Candy house Sesame
    ESP_LOGI(TAG, "initial ssm!");
    ssm_init(SSM_MAX_NUM, ssm_action_handle);

    //Init Ble Central Host
    ESP_LOGI(TAG, "initial ble host!");
    esp_ble_init();

    //Init UDP server
    ESP_LOGI(TAG, "initial gateway main!");
    iot_gateway_main_init();

    ESP_LOGI(TAG, "iot_gateway finished");
}
