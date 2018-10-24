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

#ifndef _RING_BUF_H_
#define _RING_BUF_H_

#include <stdint.h>


typedef struct{
  uint8_t* p_o;        /**< Original pointer */
  uint8_t* volatile p_r;   /**< Read pointer */
  uint8_t* volatile p_w;   /**< Write pointer */
  volatile int32_t fill_cnt;  /**< Number of filled slots */
  int32_t size;       /**< Buffer size */
  int32_t block_size;
}RINGBUF;

int32_t mqtt_rb_init(RINGBUF *r, uint8_t* buf, int32_t size, int32_t block_size);
int32_t mqtt_rb_put(RINGBUF *r, uint8_t* c);
int32_t mqtt_rb_get(RINGBUF *r, uint8_t* c);
int32_t mqtt_rb_available(RINGBUF *r);
uint32_t mqtt_rb_read(RINGBUF *r, uint8_t *buf, int len);
uint32_t mqtt_rb_write(RINGBUF *r, uint8_t *buf, int len);

#endif
