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

#ifndef HTTPD_H
#define HTTPD_H

#include "httpd-platform.h"
#include "lwip/sockets.h"

#define HTTPD_STACKSIZE (1024*4)
#define WEB_SERVER_TASK_PRIO 5

#define HTTPDVER "0.4"

#define HTTPD_MAX_CONNECTIONS 20
//Max length of request head. This is statically allocated for each connection.
#define HTTPD_MAX_HEAD_LEN      1024
//Max post buffer len. This is dynamically malloc'ed if needed.
#define HTTPD_MAX_POST_LEN      2048
//Max send buffer len. This is allocated on the stack.
#define HTTPD_MAX_SENDBUFF_LEN  8192
//If some data can't be sent because the underlaying socket doesn't accept the data (like the nonos
//layer is prone to do), we put it in a backlog that is dynamically malloc'ed. This defines the max
//size of the backlog.
#define HTTPD_MAX_BACKLOG_SIZE  (4*1024)

#define HTTPD_CGI_MORE 0
#define HTTPD_CGI_DONE 1
#define HTTPD_CGI_NOTFOUND 2
#define HTTPD_CGI_AUTHENTICATED 3

#define HTTPD_METHOD_GET 1
#define HTTPD_METHOD_POST 2
#define HTTPD_METHOD_SUB 3
#define HTTPD_METHOD_UNSUB 4
#define HTTPD_METHOD_ANNOUNCE 5
#define HTTPD_METHOD_SETUP 6
#define HTTPD_METHOD_RECORD 7
#define HTTPD_METHOD_PAUSE 8
#define HTTPD_METHOD_FLUSH 9
#define HTTPD_METHOD_TEARDOWN 10
#define HTTPD_METHOD_OPTIONS 11
#define HTTPD_METHOD_GET_PARAMETER 12
#define HTTPD_METHOD_SET_PARAMETER 13
#define HTTPD_METHOD_NOTIFY 14

#define HTTPD_TRANSFER_CLOSE 0
#define HTTPD_TRANSFER_CHUNKED 1
#define HTTPD_TRANSFER_NONE 2

typedef struct HttpdPriv HttpdPriv;
typedef struct HttpdConnData HttpdConnData;
typedef struct HttpdPostData HttpdPostData;

typedef int (* cgiSendCallback)(HttpdConnData *connData);
typedef int (* cgiRecvHandler)(HttpdConnData *connData, char *data, int len);

//A struct describing a http connection. This gets passed to cgi functions.
struct HttpdConnData {
    void *context;
    ConnTypePtr conn;       // The TCP connection. Exact type depends on the platform.
    char requestType;       // One of the HTTPD_METHOD_* values
    char *url;              // The URL requested, without hostname or GET arguments
    char *getArgs;          // The GET arguments for this request, if any.
    const void *cgiArg;     // Argument to the CGI function, as stated as the 3rd argument of
                            // the builtInUrls entry that referred to the CGI function.
    void *cgiData;          // Opaque data pointer for the CGI function
    char *hostName;         // Host name field of request
    HttpdPriv *priv;        // Opaque pointer to data for internal httpd housekeeping
    cgiSendCallback cgi;    // CGI function pointer
    cgiRecvHandler recvHdl; // Handler for data received after headers, if any
    HttpdPostData *post;    // POST data structure
    int remote_port;        // Remote TCP port
    uint8_t remote_ip[4];       // IP address of client
    uint8_t slot;               // Slot ID
};

//A struct describing the POST data sent inside the http connection.  This is used by the CGI functions
struct HttpdPostData {
    int len;                // POST Content-Length
    int buffSize;           // The maximum length of the post buffer
    int buffLen;            // The amount of bytes in the current post buffer
    int received;           // The total amount of bytes received so far
    char *buff;             // Actual POST data buffer
    char *multipartBoundary; //Text of the multipart boundary, if any
};

//A struct describing an url. This is the main struct that's used to send different URL requests to
//different routines.
typedef struct {
    const char *url;
    cgiSendCallback cgiCb;
    const void *cgiArg;
} HttpdBuiltInUrl;

typedef struct {
    HttpdBuiltInUrl *ptr;
    HttpdBuiltInUrl *next;
    void *context;
} HttpdBuiltInUrlTree;

int cgiRedirect(HttpdConnData *connData);
int cgiRedirectToHostname(HttpdConnData *connData);
int cgiRedirectApClientToHostname(HttpdConnData *connData);
void httpdRedirect(HttpdConnData *conn, char *newUrl);
int httpdUrlDecode(char *val, int valLen, char *ret, int retLen);
int httpdFindArg(char *line, char *arg, char *buff, int buffLen);
void httpdInit(int port);
const char *httpdGetMimetype(char *url);
void httdSetTransferMode(HttpdConnData *conn, int mode);
void httpdStartResponse(HttpdConnData *conn, int code);
void httpdHeader(HttpdConnData *conn, const char *field, const char *val);
void httpdEndHeaders(HttpdConnData *conn);
int httpdGetHeader(HttpdConnData *conn, char *header, char *ret, int retLen);
int httpdSend(HttpdConnData *conn, const char *data, int len);
void httpdFlushSendBuffer(HttpdConnData *conn);
void httpdContinue(HttpdConnData *conn);
void httpdConnSendStart(HttpdConnData *conn);
void httpdConnSendFinish(HttpdConnData *conn);

//Platform dependent code should call these.
void httpdSentCb(ConnTypePtr conn, char *remIp, int remPort);
void httpdRecvCb(ConnTypePtr conn, char *remIp, int remPort, char *data, unsigned short len);
void httpdDisconCb(ConnTypePtr conn, char *remIp, int remPort);
int httpdConnectCb(ConnTypePtr conn, char *remIp, int remPort);
void httpdAddRouter(HttpdBuiltInUrl router[], void *context);

#endif
