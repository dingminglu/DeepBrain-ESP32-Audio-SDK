/* http_api.h
**
** Copyright (C) 2005-2009 Collecta, Inc. 
**
**  This software is provided AS-IS with no warranty, either express
**  or implied.
**
**  This program is dual licensed under the MIT and GPLv3 licenses.
*/

#ifndef __HTTP_API_H__
#define __HTTP_API_H__

#include <stdio.h>
#include "ctypes_interface.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

char* http_get_body(const char *str_body);
int http_get_error_code(const char *str_body);
int http_get_content_length(const char *str_body);
char* http_get_chunked_body(char *str_dest, int dest_len, char *str_src);
int http_get_content_type(
	char *str_content_type,
	uint32_t str_content_type_len,
	const char *str_body);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif /* __HTTP_API_H__ */


