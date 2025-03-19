#ifndef __IOT_GATEWAY_MAIN_H__
#define __IOT_GATEWAY_MAIN_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Macro defination */
#define UDP_PORT 8889

typedef enum{
    VENDOR_NONE_ENUM = 0,
    CANDY_HOUSE_ENUM,                           /* Candy house, sesame serials*/
}vendor_e;

typedef enum {
    ACTION_NONE_ENUM = 0,
    SESAME_5_ACTION_LOCK_ENUM,                  /*<! Lock Sesame5 */
    SESAME_5_ACTION_UNLOCK_ENUM,                /*<! Unlock Sesame5 */
} candy_house_actions_e;


/* Data Structure Define */
typedef struct {
    uint8_t vendor_info[4];
    uint8_t iot_device_actions[4];
    uint8_t device_uuid[16];
    uint8_t addr[6];
    uint8_t conn_id;
    uint8_t reserve[1]; 
}iot_operation_message_t;    

/* Funtions Declare */
void iot_gateway_main_init(void);


#ifdef __cplusplus
}
#endif

#endif // __IOT_GATEWAY_MAIN_H__

