#include <stdio.h>
#include <string.h>
#include "blecent.h"
#include "nvs_flash.h"
#include "ssm_cmd.h"
#include "esp_console.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"
#include "nvs.h"
//#include "cmd_system.h"
//#include "cmd_wifi.h"
//#include "cmd_nvs.h"

/* Global Defines*/
#define PROMPT_STR CONFIG_IDF_TARGET
#define CONFIG_CONSOLE_MAX_COMMAND_LINE_LENGTH 1024

/* Global Varibal */
static const char * TAG = "iotGateway-cmd.c";

/* Functions */
static int ssm5_lock_cmd(int argc, char **argv){
	ssm_lock(NULL,0);
    return ESP_OK;
}

static int ssm5_unlock_cmd(int argc, char **argv){
	ssm_unlock(NULL,0);
    return ESP_OK;
}

static void ssm_register_console_cmd(void)
{
    const esp_console_cmd_t cmd = {
        .command = "ssm5_lock",
    	.help = "Lock ssm5",
        .hint = NULL,
        .func = ssm5_lock_cmd,
	};
	
    if(ESP_OK != esp_console_cmd_register(&cmd)){
		ESP_LOGI(TAG, "Register Lock command failed!");
    }
    
    const esp_console_cmd_t cmd1 = {
        .command = "ssm5_unlock",
    	.help = "Unlock ssm5",
        .hint = NULL,
        .func = ssm5_unlock_cmd,
	};
	
    if(ESP_OK != esp_console_cmd_register(&cmd1)){
		ESP_LOGI(TAG, "Register Lock command failed!");
    }
}


void iot_gateway_console_init(void)
{
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    repl_config.prompt = PROMPT_STR ">";
    repl_config.max_cmdline_length = CONFIG_CONSOLE_MAX_COMMAND_LINE_LENGTH;

    /* nvs already inited in app_main*/
    //initialize_nvs();

#if CONFIG_CONSOLE_STORE_HISTORY
    initialize_filesystem();
    repl_config.history_save_path = HISTORY_PATH;
    ESP_LOGI(TAG, "Command history enabled");
#else
    ESP_LOGI(TAG, "Command history disabled");
#endif

    /* Register commands */
    esp_console_register_help_command();
	ssm_register_console_cmd();

    //register_system_common();
    
#if SOC_LIGHT_SLEEP_SUPPORTED
    //register_system_light_sleep();
#endif
#if SOC_DEEP_SLEEP_SUPPORTED
    //register_system_deep_sleep();
#endif

    /* Now, it is not time to enable wifi */
#if (CONFIG_ESP_WIFI_ENABLED || CONFIG_ESP_HOST_WIFI_ENABLED)
    //register_wifi();
#endif
    /* Leon: register nvs command. function name is not good */
    //register_nvs();

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));

#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
    esp_console_dev_usb_cdc_config_t hw_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&hw_config, &repl_config, &repl));

#elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));

#else
#error Unsupported console type
#endif

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

