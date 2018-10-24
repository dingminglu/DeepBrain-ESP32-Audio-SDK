/*
 * Copyright 2017-2018 deepbrain.ai, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://deepbrain.ai/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
 
#include <string.h>
#include "app_ring_buffer.h"
#include "memory_interface.h"
#include "debug_log_interface.h"
#include "semaphore_lock_interface.h"

#define LOG_TAG "app-ring-buffer"

//环形缓冲数据存储对象
typedef struct APP_RING_BUFFER_t
{
	uint32_t curr_len;			//当前存储数据长度
	uint8_t* read_addr;			//读地址
	uint8_t* write_addr;		//写地址
	uint32_t ring_buffer_len;	//ring buffer 环形缓冲区大小
	uint8_t *ring_buffer;		//数据存储buffer，大小ring_buffer_len
	void *mutex_lock;			//资源互斥锁
}APP_RING_BUFFER_t;

APP_FRAMEWORK_ERRNO_t app_ring_buffer_new(
	const uint32_t ring_buffer_size, 
	void **ring_buffer)
{
	APP_RING_BUFFER_t *app_ring_buffer = NULL;
	
	if (ring_buffer_size == 0 
		|| ring_buffer == NULL)
	{
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	app_ring_buffer = (APP_RING_BUFFER_t *)memory_malloc(sizeof(APP_RING_BUFFER_t));
	if (app_ring_buffer == NULL)
	{
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	memset(app_ring_buffer, 0, sizeof(APP_RING_BUFFER_t));

	app_ring_buffer->ring_buffer_len = ring_buffer_size;
	app_ring_buffer->ring_buffer = (uint8_t *)memory_malloc(ring_buffer_size);
	if (app_ring_buffer->ring_buffer == NULL)
	{
		app_ring_buffer_delete(app_ring_buffer);
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;		
	}

	SEMPHR_CREATE_LOCK(app_ring_buffer->mutex_lock);
	if (app_ring_buffer->mutex_lock == NULL)
	{
		app_ring_buffer_delete(app_ring_buffer);
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;		
	}

	app_ring_buffer->write_addr = app_ring_buffer->ring_buffer;
	app_ring_buffer->read_addr = app_ring_buffer->ring_buffer;

	*ring_buffer = app_ring_buffer;
	
	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t app_ring_buffer_delete(void *ring_buffer)
{
	APP_RING_BUFFER_t *app_ring_buffer = (APP_RING_BUFFER_t *)ring_buffer;

	if (app_ring_buffer == NULL)
	{
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	if (app_ring_buffer->ring_buffer != NULL)
	{
		memory_free(app_ring_buffer->ring_buffer);
		app_ring_buffer->ring_buffer = NULL;
	}

	if (app_ring_buffer->mutex_lock != NULL)
	{
		SEMPHR_DELETE_LOCK(app_ring_buffer->mutex_lock);
		app_ring_buffer->mutex_lock = NULL;
	}
	
	memory_free(app_ring_buffer);
	app_ring_buffer = NULL;
	
	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t app_ring_buffer_clear(void *ring_buffer)
{
	APP_RING_BUFFER_t *app_ring_buffer = (APP_RING_BUFFER_t *)ring_buffer;

	if (app_ring_buffer == NULL)
	{
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	SEMPHR_TRY_LOCK(app_ring_buffer->mutex_lock);
	app_ring_buffer->curr_len = 0;
	app_ring_buffer->write_addr = app_ring_buffer->ring_buffer;
	app_ring_buffer->read_addr = app_ring_buffer->ring_buffer;
	SEMPHR_TRY_UNLOCK(app_ring_buffer->mutex_lock);
	
	return APP_FRAMEWORK_ERRNO_OK;
}

int app_ring_buffer_read(
	void *ring_buffer, 
	uint8_t *data, 
	const uint32_t size)
{	
	APP_RING_BUFFER_t *app_ring_buffer = (APP_RING_BUFFER_t *)ring_buffer;
	uint32_t read_len = 0;
	uint32_t left_read_len = 0;

	if (app_ring_buffer == NULL
		|| data == NULL
		|| size == 0)
	{
		return -1;
	}
	
	SEMPHR_TRY_LOCK(app_ring_buffer->mutex_lock);
	//判断是否有可读数据
	//DEBUG_LOGE(LOG_TAG, "curr_len[%d]", app_ring_buffer->curr_len);
	if (app_ring_buffer->curr_len <= 0)
	{
		SEMPHR_TRY_UNLOCK(app_ring_buffer->mutex_lock);
		return 0;
	}

	//计算可读数据长度
	read_len = app_ring_buffer->curr_len >= size ? size : app_ring_buffer->curr_len;
	
	//读取缓冲区
	left_read_len = app_ring_buffer->ring_buffer_len - abs(app_ring_buffer->read_addr - app_ring_buffer->ring_buffer);
	//DEBUG_LOGE(LOG_TAG, "read_buffer_ring_data:read_len[%d], size[%d], left_read_len[%d]", 
	//	read_len, size, left_read_len);
	if (left_read_len > read_len)
	{
		memcpy(data, app_ring_buffer->read_addr, read_len);
		app_ring_buffer->read_addr += read_len;
		app_ring_buffer->curr_len -= read_len;
	}
	else if (left_read_len == read_len)
	{
		memcpy(data, app_ring_buffer->read_addr, read_len);
		app_ring_buffer->read_addr = app_ring_buffer->ring_buffer;
		app_ring_buffer->curr_len -= read_len;
	}
	else
	{
		memcpy(data, app_ring_buffer->read_addr, left_read_len);
		memcpy(data + left_read_len, app_ring_buffer->ring_buffer, read_len - left_read_len);
		app_ring_buffer->read_addr = app_ring_buffer->ring_buffer + read_len - left_read_len;
		app_ring_buffer->curr_len -= read_len;
	}
	
	SEMPHR_TRY_UNLOCK(app_ring_buffer->mutex_lock);

	return read_len;
}

int app_ring_buffer_readn(
	void *ring_buffer, 
	uint8_t *data, 
	const uint32_t size)
{	
	APP_RING_BUFFER_t *app_ring_buffer = (APP_RING_BUFFER_t *)ring_buffer;
	uint32_t read_len = 0;
	uint32_t left_read_len = 0;

	if (app_ring_buffer == NULL
		|| data == NULL
		|| size == 0)
	{
		return -1;
	}
	
	SEMPHR_TRY_LOCK(app_ring_buffer->mutex_lock);
	//判断是否有可读数据
	//DEBUG_LOGE(LOG_TAG, "curr_len[%d]", app_ring_buffer->curr_len);
	if (app_ring_buffer->curr_len < size)
	{
		SEMPHR_TRY_UNLOCK(app_ring_buffer->mutex_lock);
		return 0;
	}

	//计算可读数据长度
	read_len = app_ring_buffer->curr_len >= size ? size : app_ring_buffer->curr_len;
	
	//读取缓冲区
	left_read_len = app_ring_buffer->ring_buffer_len - abs(app_ring_buffer->read_addr - app_ring_buffer->ring_buffer);
	//DEBUG_LOGE(LOG_TAG, "read_buffer_ring_data:read_len[%d], size[%d], left_read_len[%d]", 
	//	read_len, size, left_read_len);
	if (left_read_len > read_len)
	{
		memcpy(data, app_ring_buffer->read_addr, read_len);
		app_ring_buffer->read_addr += read_len;
		app_ring_buffer->curr_len -= read_len;
	}
	else if (left_read_len == read_len)
	{
		memcpy(data, app_ring_buffer->read_addr, read_len);
		app_ring_buffer->read_addr = app_ring_buffer->ring_buffer;
		app_ring_buffer->curr_len -= read_len;
	}
	else
	{
		memcpy(data, app_ring_buffer->read_addr, left_read_len);
		memcpy(data + left_read_len, app_ring_buffer->ring_buffer, read_len - left_read_len);
		app_ring_buffer->read_addr = app_ring_buffer->ring_buffer + read_len - left_read_len;
		app_ring_buffer->curr_len -= read_len;
	}
	
	SEMPHR_TRY_UNLOCK(app_ring_buffer->mutex_lock);

	return read_len;
}


int app_ring_buffer_write(
	void *ring_buffer, 
	const uint8_t *data, 
	const uint32_t size)
{	
	APP_RING_BUFFER_t *app_ring_buffer = (APP_RING_BUFFER_t *)ring_buffer;
	uint32_t write_len = 0;
	uint32_t left_write_len = 0;

	if (app_ring_buffer == NULL
		|| data == NULL
		|| size == 0)
	{
		return -1;
	}
	
	SEMPHR_TRY_LOCK(app_ring_buffer->mutex_lock);
	//判断是否还可以写入数据
	if (app_ring_buffer->curr_len >= app_ring_buffer->ring_buffer_len)
	{
		SEMPHR_TRY_UNLOCK(app_ring_buffer->mutex_lock);
		return 0;
	}
	
	//计算可写入数据长度
	write_len = app_ring_buffer->ring_buffer_len - app_ring_buffer->curr_len;
	write_len = size <= write_len ? size : write_len;
	
	//写环形缓冲区
	left_write_len = app_ring_buffer->ring_buffer_len - abs(app_ring_buffer->write_addr - app_ring_buffer->ring_buffer);
	//DEBUG_LOGE(LOG_TAG, "write_buffer_ring_data:write_len[%d], size[%d], left_write_len[%d]", 
	//	write_len, size, left_write_len);
	if (left_write_len > write_len)
	{
		memcpy(app_ring_buffer->write_addr, data, write_len);
		app_ring_buffer->write_addr += write_len;
		app_ring_buffer->curr_len += write_len;
	}
	else if (left_write_len == write_len)
	{
		memcpy(app_ring_buffer->write_addr, data, write_len);
		app_ring_buffer->write_addr = app_ring_buffer->ring_buffer;
		app_ring_buffer->curr_len += write_len;
	}
	else
	{
		memcpy(app_ring_buffer->write_addr, data, left_write_len);
		memcpy(app_ring_buffer->ring_buffer, data+left_write_len, write_len - left_write_len);
		app_ring_buffer->write_addr = app_ring_buffer->ring_buffer + write_len - left_write_len;
		app_ring_buffer->curr_len += write_len;
	}
	
	SEMPHR_TRY_UNLOCK(app_ring_buffer->mutex_lock);

	return write_len;
}

