/* sock.h
** socket abstraction header
**
** Copyright (C) 2005-2009 Collecta, Inc. 
**
**  This software is provided AS-IS with no warranty, either express
**  or implied.
**
**  This program is dual licensed under the MIT and GPLv3 licenses.
*/

/** @file
 *  Socket abstraction API.
 */

#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <stdio.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"


#ifdef __cplusplus
extern "C" {
#endif /* C++ */

#define INVALID_SOCK -1
typedef int sock_t;

int sock_get_server_info(const char *str_web_url, char *str_domain, char *str_port, char *str_params);
sock_t sock_connect(const char * const host, const char * const port);
int sock_close(const sock_t sock);
int sock_set_blocking(const sock_t sock);
int sock_set_nonblocking(const sock_t sock);
int sock_set_read_timeout(const sock_t sock, const unsigned int sec, const unsigned int usec);
int sock_set_write_timeout(const sock_t sock, const unsigned int sec, const unsigned int usec);
int sock_read(const sock_t sock, void * const buff, const size_t len);
int sock_readn(const sock_t sock, void * const buff, const size_t len);
int sock_write(const sock_t sock, const void * const buff, const size_t len);
int sock_writen(const sock_t sock, const void * const buff, const size_t len);
int sock_writen_with_timeout(const sock_t sock, const void * const buff, const size_t len, const size_t ms);
int sock_readn_with_timeout(
	const sock_t sock, void * const buff, const size_t len, const size_t ms);
int sock_get_errno(const sock_t sock);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif /* __SOCKET_H__ */
