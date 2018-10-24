/* HTTP GET Example using plain POSIX sockets

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_audio_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "sdkconfig.h"
#include "airkiss.h"
#ifdef CONFIG_ENABLE_TURINGWECHAT_SERVICE
#include "TuringRobotService.h"
#define AIRKISS_SEND_NOTIFY 1

const airkiss_config_t akconf = {
    (airkiss_memset_fn)& memset,
    (airkiss_memcpy_fn)& memcpy,
    (airkiss_memcmp_fn)& memcmp,
    (airkiss_printf_fn)& printf,
};


//#define DEVICE_TYPE "gh_ca1f565e168e" //wechat public number
#define DEVICE_TYPE "gh_ca69fb525566" //wechat public number for new
#define DEFAULT_LAN_PORT 12476

extern TaskHandle_t airkiss_notify_handle;
void airkiss_notify_wechat(void* parame)
{
    struct sockaddr_in addr;
    struct sockaddr_in to_addr;
    socklen_t addr_len;
    int i, ret, fd;
    int buf_len = 200;
    uint8_t buf[200];
    uint16_t lan_buf_len;
    unsigned short recv_len;
    struct sockaddr_in send_addr;
    uint8_t lan_buf[200];
    uint16_t send_buf_len;
    int send_fd;

    do {
        send_fd = socket(AF_INET, SOCK_DGRAM, 0);

        if (send_fd == -1) {
            printf("ERROR: failed to create sock!\n");
            vTaskDelete(NULL);
        }
    } while (send_fd == -1);

    memset(&send_addr, 0, sizeof(send_addr));
    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = INADDR_BROADCAST;
    send_addr.sin_port = htons(DEFAULT_LAN_PORT);
    send_addr.sin_len = sizeof(send_addr);
    memset(lan_buf, 0, sizeof(lan_buf));
    send_buf_len = sizeof(lan_buf);
    char airkiss_device_id[256] = {0};
    sprintf(airkiss_device_id, "%s_%s",API_KEY,DEVICE_ID);
    ret = airkiss_lan_pack(AIRKISS_LAN_SSDP_NOTIFY_CMD,
                           DEVICE_TYPE, airkiss_device_id, 0, 0, lan_buf, &send_buf_len, &akconf);

    if (ret != AIRKISS_LAN_PAKE_READY) {
        printf("Pack lan packet error!");
        vTaskDelete(NULL);
    }

    do {
        fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);//IPPROTO_UDP

        if (fd == -1) {
            printf("ERROR: failed to create sock!\n");
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
    } while (fd == -1);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(DEFAULT_LAN_PORT);
    addr.sin_len = sizeof(addr);
    ret = bind(fd, (const struct sockaddr*)&addr, sizeof(addr));

    if (ret) {
        printf("aws bind local port ERROR!\r\n");
    }

    //send notification
    int send_num = 20;

    while (send_num--) {
        struct timeval tv;
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        ret = select(fd + 1, &rfds, NULL, NULL, &tv);

        if (ret > 0) {
            memset(buf, 0 , buf_len);
            addr_len = sizeof(to_addr);
            recv_len = recvfrom(fd, buf, buf_len, 0,
                                (struct sockaddr*)&to_addr, (socklen_t*)&addr_len);

            if (ret) {
                airkiss_lan_ret_t ret = airkiss_lan_recv(buf, recv_len, &akconf);
                airkiss_lan_ret_t packret;

                switch (ret) {
                    case AIRKISS_LAN_SSDP_REQ:
                        printf("AIRKISS_LAN_SSDP_REQ\r\n");
                        lan_buf_len = sizeof(buf);
                        memset(buf, 0, sizeof(buf));
                        packret = airkiss_lan_pack(AIRKISS_LAN_SSDP_RESP_CMD,
                                                   DEVICE_TYPE, airkiss_device_id, 0, 0, buf, &lan_buf_len, &akconf);

                        if (packret != AIRKISS_LAN_PAKE_READY) {
                            printf("Pack lan packet error!");
                            vTaskDelete(NULL);
                        }

                        ret = sendto(fd, buf, lan_buf_len, 0,
                                     (struct sockaddr*)&to_addr, sizeof(to_addr));

                        if (ret < 0) {
                            printf("aws send notify msg ERROR!\r\n");
                        }

                        break;

                    default:
                        //printf("Pack is not ssdq req!%d\r\n",ret);
                        break;
                }
            }
        } else {
            ret = sendto(send_fd, lan_buf, send_buf_len, 0,
                         (const struct sockaddr*)&send_addr, sizeof(send_addr));

            if (ret < 0) {
                printf("send notify msg ERROR!\r\n");
            } else {
                printf("send notify msg OK!\r\n");
            }
        }

        //vendor_msleep(200 + i * 100);
    }

    close(fd);
    close(send_fd);
    airkiss_notify_handle = NULL;
    vTaskDelete(NULL);
}
#endif
