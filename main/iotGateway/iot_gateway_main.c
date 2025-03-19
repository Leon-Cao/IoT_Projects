#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "blecent.h"
#include "nvs_flash.h"
#include "ssm_cmd.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
//#include "protocol_examples_common.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "iot_gateway_main.h"
#include "ssm_cmd.h"


#define CONFIG_EXAMPLE_IPV4
//#define CONFIG_LWIP_NETBUF_RECVINFO


static const char *TAG = "iot_Gateway_main";

static esp_err_t iot_gateway_operation_message_processing(void* message, int msg_len)
{
    iot_operation_message_t* g_oper_msg = (iot_operation_message_t*) message;
    
    ESP_LOGI(TAG, "vendor info: %02x %02x %02x %02x",g_oper_msg->vendor_info[0],
        g_oper_msg->vendor_info[1],g_oper_msg->vendor_info[2],g_oper_msg->vendor_info[3]);
    if(CANDY_HOUSE_ENUM == g_oper_msg->vendor_info[0])
    {
        
        ESP_LOGI(TAG, "action info: %02x %02x %02x %02x",g_oper_msg->iot_device_actions[0],
            g_oper_msg->iot_device_actions[1],g_oper_msg->iot_device_actions[2],g_oper_msg->iot_device_actions[3]);
        if(SESAME_5_ACTION_LOCK_ENUM == g_oper_msg->iot_device_actions[0])
        {
            ssm_lock(NULL,0);
        }
        else if(SESAME_5_ACTION_UNLOCK_ENUM == g_oper_msg->iot_device_actions[0])
        {
            ssm_unlock(NULL,0);
        }
    }

    return ESP_OK;
}

static void iot_udp_server_task(void *pvParameters)
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    struct sockaddr_in6 dest_addr;
    //struct sockaddr_in *dest_addr_ip4;
    int udp_sock;
    

    while (1) {
        if (addr_family == AF_INET) {
            //dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
            struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
            dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
            dest_addr_ip4->sin_family = AF_INET;
            dest_addr_ip4->sin_port = htons(UDP_PORT);
            ip_protocol = IPPROTO_IP;
        } else if (addr_family == AF_INET6) {
            ESP_LOGI(TAG, "IPv6 Socket creating");
            bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
            dest_addr.sin6_family = AF_INET6;
            dest_addr.sin6_port = htons(UDP_PORT);
            ip_protocol = IPPROTO_IPV6;
        }

        udp_sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (udp_sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created！addr_family=%d, protocal=%d.", addr_family, ip_protocol);
        ESP_LOGI(TAG, "Reference: addr_family AF_INET=2, AF_INET6=10; IPPROTO_IP=0, IPPROTO_IPV6=41");
#if defined(CONFIG_LWIP_NETBUF_RECVINFO) && !defined(CONFIG_EXAMPLE_IPV6)
        int enable = 1;
        lwip_setsockopt(udp_sock, IPPROTO_IP, IP_PKTINFO, &enable, sizeof(enable));
#endif

#if defined(CONFIG_EXAMPLE_IPV4) && defined(CONFIG_EXAMPLE_IPV6)
        if (addr_family == AF_INET6) {
            // Note that by default IPV6 binds to both protocols, it is must be disabled
            // if both protocols used at the same time (used in CI)
            int opt = 1;
            setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            setsockopt(udp_sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
        }
#endif
        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 100;
        timeout.tv_usec = 0;
        setsockopt (udp_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

        int err = bind(udp_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", UDP_PORT);

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t socklen = sizeof(source_addr);

#if defined(CONFIG_LWIP_NETBUF_RECVINFO) && !defined(CONFIG_EXAMPLE_IPV6)
        struct iovec iov;
        struct msghdr msg;
        struct cmsghdr *cmsgtmp;
        u8_t cmsg_buf[CMSG_SPACE(sizeof(struct in_pktinfo))];

        iov.iov_base = rx_buffer;
        iov.iov_len = sizeof(rx_buffer);
        msg.msg_control = cmsg_buf;
        msg.msg_controllen = sizeof(cmsg_buf);
        msg.msg_flags = 0;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_name = (struct sockaddr *)&source_addr;
        msg.msg_namelen = socklen;
#endif

        while (1) {
#if defined(CONFIG_LWIP_NETBUF_RECVINFO) && !defined(CONFIG_EXAMPLE_IPV6)
            ESP_LOGI(TAG, "Waiting for raw data");
            int len = recvmsg(udp_sock, &msg, 0);
#else
            ESP_LOGI(TAG, "Waiting for data");
            int len = recvfrom(udp_sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
#endif
            ESP_LOGE(TAG, "recvfrom ..., lenght=%d.", len);
            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                // Get the sender's ip address as string
                if (source_addr.ss_family == PF_INET) {
                    inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
#if defined(CONFIG_LWIP_NETBUF_RECVINFO) && !defined(CONFIG_EXAMPLE_IPV6)
                    for ( cmsgtmp = CMSG_FIRSTHDR(&msg); cmsgtmp != NULL; cmsgtmp = CMSG_NXTHDR(&msg, cmsgtmp) ) {
                        if ( cmsgtmp->cmsg_level == IPPROTO_IP && cmsgtmp->cmsg_type == IP_PKTINFO ) {
                            struct in_pktinfo *pktinfo;
                            pktinfo = (struct in_pktinfo*)CMSG_DATA(cmsgtmp);
                            ESP_LOGI(TAG, "dest ip: %s", inet_ntoa(pktinfo->ipi_addr));
                        }
                    }
#endif
                } else if (source_addr.ss_family == PF_INET6) {
                    inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
                }

                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                ESP_LOGI(TAG, "msg= %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x:", rx_buffer[0],
                    rx_buffer[1], rx_buffer[2],rx_buffer[3],rx_buffer[4],rx_buffer[5],rx_buffer[6],rx_buffer[7],
                    rx_buffer[8],rx_buffer[9],rx_buffer[10],rx_buffer[11]);
                iot_gateway_operation_message_processing(rx_buffer, len);
                
//                ESP_LOGI(TAG, "%s", rx_buffer);

/*                int err = sendto(sock, rx_buffer, len, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
                if (err < 0) {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    break;
                }*/
            }
        }

        if (udp_sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(udp_sock, 0);
            close(udp_sock);
        }
    }
    vTaskDelete(NULL);
}

void iot_gateway_main_init(void)
{
    //ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    

#ifdef CONFIG_EXAMPLE_IPV4
    ESP_LOGE(TAG, "Create IPv4 udp server task...");
    xTaskCreate(iot_udp_server_task, "udp_server", 4096, (void*)AF_INET, 5, NULL);
#endif
#ifdef CONFIG_EXAMPLE_IPV6
    xTaskCreate(iot_udp_server_task, "udp_server", 4096, (void*)AF_INET6, 5, NULL);
#endif

}
