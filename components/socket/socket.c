/* sock.c
** strophe XMPP client library -- socket abstraction implementation
**
** Copyright (C) 2005-2009 Collecta, Inc. 
**
**  This software is provided AS-IS with no warranty, either express
**  or implied.
**
** This program is dual licensed under the MIT and GPLv3 licenses.
*/

/** @file
 *  Socket abstraction.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "time.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "socket.h"
#include "debug_log_interface.h"

#define LOG_TAG "socket"
const int MAX_SOCKET_WRITE_SIZE = 512;

int sock_get_server_info(
	const char *str_web_url, 
	char *str_domain, 
	char *str_port, 
	char *str_params)
{
	if (NULL == str_web_url)
	{
		return -1;
	}
	
	//get domain
	char* domain_start = strstr(str_web_url, "http://");
	if (NULL == domain_start)
	{
		return -1;
	}
	domain_start = domain_start + strlen("http://");
	if (*domain_start == '\0')
	{
		return -1;
	}

	//get port
	char* port_start = strstr(domain_start, ":");
	char* port_end = strstr(domain_start, "/");
	if (NULL == port_start)
	{
		strcpy(str_port, "80");//default 80 port
		if (NULL == port_end)
		{
			strcpy(str_domain, domain_start);
			return 0;
		}
		else
		{
			strncpy(str_domain, domain_start, port_end - domain_start);
		}
	}
	else
	{	
		strncpy(str_domain, domain_start, port_start - domain_start);
		port_start = port_start + 1;
		if (NULL == port_end)
		{	
			strcpy(str_port, port_start);
			return 0;
		}
		else
		{
			strncpy(str_port, port_start, port_end - port_start);
		}
	}

	//get parameters
	if (*port_end != '\0')
	{
		strcpy(str_params, port_end);	
	}

	return 0;
}

sock_t sock_connect(const char * const host, const char * const port)
{
	int err = INVALID_SOCK;
    sock_t sock = INVALID_SOCK;
    struct addrinfo *res = NULL;
	struct addrinfo	*ainfo = NULL; 
	struct addrinfo hints = 
	{
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
	};
	void *ptr = NULL;
	char ip_addr[128] = {0};

    err = getaddrinfo(host, port, &hints, &res);
    if (err != 0)
    {
    	DEBUG_LOGE(LOG_TAG, "sock_connect getaddrinfo error:[%d]\r\n", err);  
        return INVALID_SOCK;
    }

	for (ainfo = res; ainfo != NULL; ainfo = ainfo->ai_next) 
	{
		memset(ip_addr, 0, sizeof(ip_addr));
		
		switch(ainfo->ai_family)
		{
			case AF_INET:
			{
				ptr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
				break;
			}
			case AF_INET6:
			{
				ptr = &((struct sockaddr_in6 *)res->ai_addr)->sin6_addr;
				break;
			}
		}

		if (ptr != NULL)
		{
			inet_ntop(ainfo->ai_family, ptr, ip_addr, sizeof(ip_addr));
		}

		//DEBUG_LOGE(LOG_TAG, "connect,ip_addr[%s]", ip_addr);  
	    sock = socket(ainfo->ai_family, ainfo->ai_socktype, ainfo->ai_protocol);
	    if (sock < 0)
	    {
	        continue;
	    }
		sock_set_nonblocking(sock);
        err = connect(sock, ainfo->ai_addr, ainfo->ai_addrlen);
        if (err == 0)
        {
            break;
        }
		else
		{  
			err = sock_get_errno(sock);
			if(err != EINPROGRESS)
			{
				DEBUG_LOGE(LOG_TAG, "connect error :[%d]\n", err);  
			}
        	else  
	        {  
	            struct timeval tm = {2, 0};  
	            fd_set wset,rset;  
	            FD_ZERO(&wset);  
	            FD_ZERO(&rset);  
	            FD_SET(sock,&wset);  
	            FD_SET(sock,&rset);  
	            long t1 = time(NULL);  
	            int res = select(sock+1,&rset,&wset,NULL,&tm);  
	            long t2 = time(NULL);  
	            //printf("interval time: %ld, select: %d\n", t2 - t1, res);  
	            if(res < 0)  
	            {  
	                DEBUG_LOGE(LOG_TAG, "network error in connect\n");  
	            }  
	            else if(res == 0)  
	            {  
	                DEBUG_LOGE(LOG_TAG, "connect time out,ip_addr[%s]", ip_addr);  
	            }  
	            else if (res == 1)  
	            {  
	                if(FD_ISSET(sock,&wset))  
	                {   
	                    sock_set_blocking(sock);
						break;
	                }  
	                else  
	                {  
	                    DEBUG_LOGE(LOG_TAG, "other error when select\n");  
	                }  
	            }  
	        }
		}
	    sock_close(sock);
	}
	freeaddrinfo(res);
	sock = ainfo == NULL ? INVALID_SOCK : sock;

    return sock;
}

int sock_close(const sock_t sock)
{
	return close(sock);
}

int sock_set_blocking(const sock_t sock)
{
    int rc;

    rc = fcntl(sock, F_GETFL, NULL);
    if (rc >= 0)
	{
        rc = fcntl(sock, F_SETFL, rc & (~O_NONBLOCK));
    }
	
    return rc;
}

int sock_set_nonblocking(const sock_t sock)
{
    int rc;

    rc = fcntl(sock, F_GETFL, NULL);
    if (rc >= 0) 
	{
        rc = fcntl(sock, F_SETFL, rc | O_NONBLOCK);
    }
	
    return rc;
}

int sock_set_read_timeout(const sock_t sock, const unsigned int sec, const unsigned int usec)
{
	struct timeval timeout={sec, usec};
	return setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
}

int sock_set_write_timeout(const sock_t sock, const unsigned int sec, const unsigned int usec)
{
	struct timeval timeout={sec, usec};
	return setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
}

int sock_read(const sock_t sock, void * const buff, const size_t len)
{
    return recv(sock, buff, len, 0);
}

int sock_readn(const sock_t sock, void * const buff, const size_t len)
{
	int ret = 0;
	int recv_len = 0;
	
	do 
	{		
		ret = sock_read(sock, (char *)buff + recv_len, len - recv_len);
		if (ret > 0)
		{
			recv_len += ret;
		}
		else if (ret < 0)
		{
			continue;
		}
		else
		{
			break;
		}
	} while(recv_len != len);

	return recv_len;
}

uint64_t _get_ms_of_day(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return tv.tv_sec*1000 + tv.tv_usec/1000;
}

int sock_readn_with_timeout(
	const sock_t sock, void * const buff, const size_t len,
	const size_t ms)
{
	int ret = 0;
	int recv_len = 0;
	uint64_t start_time = _get_ms_of_day();
	char *p = buff;
	fd_set readfds;
	struct timeval timeout;

	if (ms >= 1000)
	{
		timeout.tv_sec  = ms/1000;
		timeout.tv_usec = ms%1000*1000;
	}
	else
	{
		timeout.tv_sec = 0;
		timeout.tv_usec = ms*1000;
	}
	
	do 
	{
		if (abs(_get_ms_of_day() - start_time) >= ms)
		{
			break;
		}
	
		FD_ZERO(&readfds);
		FD_SET(sock, &readfds);
		
		int ret = select(sock + 1, &readfds, NULL, NULL, &timeout);
		if (ret == -1) 
		{
			DEBUG_LOGE(LOG_TAG, "sock_readn_with_timeout select failed");
			break;
		} 
		else if (ret == 0) 
		{
			break;
		}
		else
		{
			if (FD_ISSET(sock, &readfds) == 0)
			{
				continue;
			}
		}
		
		ret = sock_get_errno(sock);
		if (ret != 0 && ret != EAGAIN && ret == EINTR)
		{
			break;
		}

		sock_set_read_timeout(sock, 1, 0);
		ret = sock_read(sock, (char *)p, len - recv_len);
		if (ret > 0)
		{
			recv_len += ret;
			p += ret;
		}
		else if (ret < 0)
		{
			ret = sock_get_errno(sock);
			if (ret != 0 && ret != EAGAIN && ret == EINTR)
			{
				break;
			}
			else
			{
				continue;
			}
		}
		else
		{
			break;
		}
	} while(recv_len != len);

	//printf("sock_readn_with_timeout cost %ums\r\n", abs(_get_ms_of_day() - start_time));
	
	return recv_len;
}


int sock_writen_with_timeout(
	const sock_t sock, const void * const buff, const size_t len, const size_t ms)
{
	int ret 		= -1;
	int data_len 	= len;
	int write_len 	= 0;
	int position 	= 0;
	uint64_t start_time = _get_ms_of_day();
	fd_set writefds;
	struct timeval timeout;

	if (ms >= 1000)
	{
		timeout.tv_sec  = ms/1000;
		timeout.tv_usec = ms%1000*1000;
	}
	else
	{
		timeout.tv_sec = 0;
		timeout.tv_usec = ms*1000;
	}
	
	while(data_len > 0)
	{
		FD_ZERO(&writefds);
		FD_SET(sock, &writefds);

		int ret = select(sock + 1, NULL, &writefds, NULL, &timeout);
		if (ret == -1) 
		{
			DEBUG_LOGE(LOG_TAG, "sock_writen_with_timeout select failed()\r\n");
			break;
		} 
		else if (ret == 0) 
		{
			break;
		}
		else
		{
			if (FD_ISSET(sock, &writefds) == 0)
			{
				continue;
			}
		}
		ret = sock_get_errno(sock);
		if (ret != 0 && ret != EAGAIN && ret == EINTR)
		{
			break;
		}

		if (abs(_get_ms_of_day() - start_time) >= ms)
		{
			break;
		}

		write_len = data_len <= MAX_SOCKET_WRITE_SIZE ? data_len : MAX_SOCKET_WRITE_SIZE;
		ret = send(sock, (char*)buff + position, write_len, 0);
		if (ret > 0)
		{
			position += ret;
			data_len -= ret;
		}
		else if (ret < 0)
		{
			continue;
		}
		else
		{
			break;
		}
	}

	//printf("sock_writen_with_timeout cost %ums\r\n", abs(_get_ms_of_day() - start_time));

	return position;
}

int sock_writen(const sock_t sock, const void * const buff, const size_t len)
{
	int ret 		= -1;
	int data_len	= len;
	int write_len	= 0;
	int position	= 0;
	
	while(data_len > 0)
	{
		write_len = data_len <= MAX_SOCKET_WRITE_SIZE ? data_len : MAX_SOCKET_WRITE_SIZE;
		ret = send(sock, (char*)buff + position, write_len, 0);
		if (ret > 0)
		{
			position += ret;
			data_len -= ret;
		}
		else if (ret < 0)
		{
			continue;
		}
		else
		{
			break;
		}
	}

	return position;
}


int sock_write(const sock_t sock, const void * const buff, const size_t len)
{
	int ret 		= -1;
	int data_len 	= len;
	int write_len 	= 0;
	int position 	= 0;
	
	while(data_len > 0)
	{
		write_len = data_len <= MAX_SOCKET_WRITE_SIZE ? data_len : MAX_SOCKET_WRITE_SIZE;
		ret = send(sock, (char*)buff + position, write_len, 0);
		if (ret > 0)
		{
			position += ret;
			data_len -= ret;
		}
		else if (ret < 0)
		{
			continue;
		}
		else
		{
			break;
		}
	}

	return position;
}

int sock_get_errno(const sock_t sock)
{
	int ret = 0;
	int optval = -1;
   	size_t optlen = sizeof(optval);
   
	ret = getsockopt(sock, SOL_SOCKET, SO_ERROR, &optval, &optlen);
	if (ret != 0 || optval != 0)
	{
		//DEBUG_LOGE(LOG_TAG, "getsockopt ret[%d], sock errno = %d\r\n", ret, optval);
	}
	
	return optval;
}

