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

#ifndef _LIST_H
#define _LIST_H

typedef struct req_list_t {
	void *key;
	void *value;
	struct req_list_t *next;
	struct req_list_t *prev;
} req_list_t;

void req_list_add(req_list_t *root, req_list_t *new_tree);
req_list_t *req_list_get_last(req_list_t *root);
req_list_t *req_list_get_first(req_list_t *root);
void req_list_remove(req_list_t *tree);
void req_list_clear(req_list_t *root);
req_list_t *req_list_set_key(req_list_t *root, const char *key, const char *value);
req_list_t *req_list_get_key(req_list_t *root, const char *key);
int req_list_check_key(req_list_t *root, const char *key, const char *value);
req_list_t *req_list_set_from_string(req_list_t *root, const char *data); //data = "key=value"
#endif
