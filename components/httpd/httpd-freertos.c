/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "httpd.h"
#include "httpd-platform.h"
#include "tcpip_adapter.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "EspAudioAlloc.h"
#if EN_STACK_TRACKER
#include "esp_audio_log.h"
#endif

static int httpPort;
static int httpMaxConnCt;
static xQueueHandle httpdMux;
TaskHandle_t HttdpTaskHandle;
static int HighWaterLine = 0;


static RtosConnType rconn[HTTPD_MAX_CONNECTIONS];

int httpdPlatSendData(ConnTypePtr conn, char *buff, int len) {
    conn->needWriteDoneNotif=1;
    return (write(conn->fd, buff, len)>=0);
}

void httpdPlatDisconnect(ConnTypePtr conn) {
    conn->needsClose=1;
    conn->needWriteDoneNotif=1; //because the real close is done in the writable select code
}

void httpdPlatDisableTimeout(ConnTypePtr conn) {
    //Unimplemented for FreeRTOS
}

//Set/clear global httpd lock.
void httpdPlatLock() {
    xSemaphoreTakeRecursive(httpdMux, portMAX_DELAY);
}

void httpdPlatUnlock() {
    xSemaphoreGiveRecursive(httpdMux);
}

static int create_listener(int port)
{
    int ret, listenfd;
    int yes = 1;
    struct sockaddr_in server_addr;
    /* Construct local address structure */
    memset(&server_addr, 0, sizeof(server_addr)); /* Zero out structure */
    server_addr.sin_family = AF_INET;           /* Internet address family */
    server_addr.sin_addr.s_addr = INADDR_ANY;   /* Any incoming interface */
    server_addr.sin_len = sizeof(server_addr);
    server_addr.sin_port = htons(port); /* Local port */

    /* Create socket for incoming connections */
    do{
        listenfd = socket(AF_INET, SOCK_STREAM, 0);
        if (listenfd == -1) {
            httpd_printf("platHttpServerTask: failed to create sock!\n");
            vTaskDelay(1000/portTICK_RATE_MS);
        }
    } while(listenfd == -1);

    fcntl(listenfd, F_SETFD, FD_CLOEXEC);
    ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    httpd_printf("SOL_SOCKET, SO_REUSEADDR = %d\n", ret);

    /* Bind to the local port */
    do{
        ret = bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (ret != 0) {
            httpd_printf("platHttpServerTask: failed to bind!\n");
            vTaskDelay(1000/portTICK_RATE_MS);
        }
    } while(ret != 0);

    do{
        /* Listen to the local connection */
        ret = listen(listenfd, HTTPD_MAX_CONNECTIONS);
        if (ret != 0) {
            httpd_printf("platHttpServerTask: failed to listen!\n");
            vTaskDelay(1000/portTICK_RATE_MS);
        }

    } while(ret != 0);
    return listenfd;
}

#define RECV_BUF_SIZE 2048
static void platHttpServerTask(void *pvParameters) {
    int32_t listenfd;
    int32_t remotefd;
    int32_t len;
    int32_t ret;
    int x;
    int maxfdp = 0;
    char *precvbuf;
    int yes = 1;
    fd_set readset,writeset;
    struct sockaddr name;
    //struct timeval timeout;
    struct sockaddr_in remote_addr;


    httpdMux=xSemaphoreCreateRecursiveMutex();

    for (x=0; x<HTTPD_MAX_CONNECTIONS; x++) {
        rconn[x].fd=-1;
    }

    listenfd = create_listener(httpPort);


    httpd_printf("esphttpd: active and listening to connections.\n");
    while(1){
        // clear fdset, and set the select function wait time
        int socketsFull=1;
        maxfdp = 0;
        FD_ZERO(&readset);
        FD_ZERO(&writeset);
        //timeout.tv_sec = 2;
        //timeout.tv_usec = 0;

        for(x=0; x<HTTPD_MAX_CONNECTIONS; x++){
            if (rconn[x].fd!=-1) {
                FD_SET(rconn[x].fd, &readset);
                if (rconn[x].needWriteDoneNotif) FD_SET(rconn[x].fd, &writeset);
                if (rconn[x].fd>maxfdp) maxfdp=rconn[x].fd;
            } else {
                socketsFull=0;
            }
        }

        if (!socketsFull) {
            FD_SET(listenfd, &readset);
            if (listenfd>maxfdp) maxfdp=listenfd;
        }

        //polling all exist client handle,wait until readable/writable
        ret = select(maxfdp+1, &readset, &writeset, NULL, NULL);//&timeout
        if(ret > 0){
            //See if we need to accept a new connection
            if (FD_ISSET(listenfd, &readset)) {
                len=sizeof(struct sockaddr_in);
                remotefd = accept(listenfd, (struct sockaddr *)&remote_addr, (socklen_t *)&len);
                if (remotefd<0) {
                    httpd_printf("platHttpServerTask: Huh? Accept failed.\n");
                    continue;
                }
                for(x=0; x<HTTPD_MAX_CONNECTIONS; x++) if (rconn[x].fd==-1) break;
                if (x==HTTPD_MAX_CONNECTIONS) {
                    httpd_printf("platHttpServerTask: Huh? Got accept with all slots full.\n");
                    continue;
                }
                int keepAlive = 1; //enable keepalive
                int keepIdle = 60; //60s
                int keepInterval = 5; //5s
                int keepCount = 3; //retry times

                setsockopt(remotefd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));
                setsockopt(remotefd, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));
                setsockopt(remotefd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));
                setsockopt(remotefd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount));

                rconn[x].fd=remotefd;
                rconn[x].needWriteDoneNotif=0;
                rconn[x].needsClose=0;

                len=sizeof(name);
                getpeername(remotefd, &name, (socklen_t *)&len);
                struct sockaddr_in *piname=(struct sockaddr_in *)&name;

                rconn[x].port=piname->sin_port;
                memcpy(&rconn[x].ip, &piname->sin_addr.s_addr, sizeof(rconn[x].ip));

                tcpip_adapter_ip_info_t ip_info;
                tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info);
                rconn[x].server_addr.sin_addr.s_addr = ip_info.ip.addr;
                // memcpy(&rconn[x].server_addr, &ip_info.ip, sizeof(server_addr));
                memcpy(&rconn[x].remote_addr, &remote_addr, sizeof(remote_addr));

                httpdConnectCb(&rconn[x], rconn[x].ip, rconn[x].port);
                //os_timer_disarm(&connData[x].conn->stop_watch);
                //os_timer_setfn(&connData[x].conn->stop_watch, (os_timer_func_t *)httpserver_conn_watcher, connData[x].conn);
                //os_timer_arm(&connData[x].conn->stop_watch, STOP_TIMER, 0);
             // httpd_printf("httpserver acpt index %d sockfd %d!\n", x, remotefd);
            }

            //See if anything happened on the existing connections.
            for(x=0; x < HTTPD_MAX_CONNECTIONS; x++){
                //Skip empty slots
                if (rconn[x].fd==-1) continue;

                //Check for write availability first: the read routines may write needWriteDoneNotif while
                //the select didn't check for that.
                if (rconn[x].needWriteDoneNotif && FD_ISSET(rconn[x].fd, &writeset)) {
                    rconn[x].needWriteDoneNotif=0; //Do this first, httpdSentCb may write something making this 1 again.
                    if (rconn[x].needsClose) {
                        //Do callback and close fd.
                        httpdDisconCb(&rconn[x], rconn[x].ip, rconn[x].port);
                        close(rconn[x].fd);
                        rconn[x].fd=-1;
                    } else {
                        httpdSentCb(&rconn[x], rconn[x].ip, rconn[x].port);
                    }
                }

                if (FD_ISSET(rconn[x].fd, &readset)) {
                    precvbuf=(char *)EspAudioAlloc(1, RECV_BUF_SIZE);
                    if (precvbuf==NULL) {
                        httpd_printf("platHttpServerTask: memory exhausted!\n");
                        httpdDisconCb(&rconn[x], rconn[x].ip, rconn[x].port);
                        close(rconn[x].fd);
                        rconn[x].fd=-1;
                    }
                    ret=recv(rconn[x].fd, precvbuf, RECV_BUF_SIZE,0);
                    if (ret > 0) {
                        //Data received. Pass to httpd.
                        httpdRecvCb(&rconn[x], rconn[x].ip, rconn[x].port, precvbuf, ret);
                    } else {
                        //recv error,connection close
                        httpdDisconCb(&rconn[x], rconn[x].ip, rconn[x].port);
                        close(rconn[x].fd);
                        rconn[x].fd=-1;
                    }
                    if (precvbuf) free(precvbuf);
                }
            }
        } else {
            httpd_printf("platHttpServerTask: we got something wrong\n");
            close(listenfd);
            listenfd = create_listener(httpPort);
            vTaskDelay(500/portTICK_RATE_MS);
        }
#if EN_STACK_TRACKER
        if(uxTaskGetStackHighWaterMark(HttdpTaskHandle) > HighWaterLine){
            HighWaterLine = uxTaskGetStackHighWaterMark(HttdpTaskHandle);
            ESP_AUDIO_LOGI("STACK", "platHttpServerTask %d\n\n\n", HighWaterLine);
        }
#endif
    }

#if 0
//Deinit code, not used here.
    /*release data connection*/
    for(x=0; x < HTTPD_MAX_CONNECTIONS; x++){
        //find all valid handle
        if(connData[x].conn == NULL) continue;
        if(connData[x].conn->sockfd >= 0){
            os_timer_disarm((os_timer_t *)&connData[x].conn->stop_watch);
            close(connData[x].conn->sockfd);
            connData[x].conn->sockfd = -1;
            connData[x].conn = NULL;
            if(connData[x].cgi!=NULL) connData[x].cgi(&connData[x]); //flush cgi data
            httpdRetireConn(&connData[x]);
        }
    }
    /*release listen socket*/
    close(listenfd);

    vTaskDelete(NULL);
#endif
}



//Initialize listening socket, do general initialization
void httpdPlatInit(int port, int maxConnCt) {
    httpPort=port;
    httpMaxConnCt=maxConnCt;
    xTaskCreatePinnedToCore(platHttpServerTask, (const char *)"upnp_esphttpd", HTTPD_STACKSIZE, NULL, WEB_SERVER_TASK_PRIO, &HttdpTaskHandle, 0);
}
