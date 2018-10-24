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

/**
* \file
*   Ring Buffer library
*/
#include <stdio.h>
#include <string.h>
#include "mqtt_ringbuf.h"

/**
* \brief init a RINGBUF object
* \param r pointer to a RINGBUF object
* \param buf pointer to a byte array
* \param size size of buf
* \param block_size is size of data as block
* \return 0 if successfull, otherwise failed
*/
int32_t mqtt_rb_init(RINGBUF *r, uint8_t* buf, int32_t size, int32_t block_size)
{
    if (r == 0 || buf == 0 || size < 2) return -1;

    if (size % block_size != 0) return -1;

    r->p_o = r->p_r = r->p_w = buf;
    r->fill_cnt = 0;
    r->size = size;
    r->block_size = block_size;
    return 0;
}
/**
* \brief put a character into ring buffer
* \param r pointer to a ringbuf object
* \param c character to be put
* \return 0 if successfull, otherwise failed
*/
int32_t mqtt_rb_put(RINGBUF *r, uint8_t *c)
{
    int32_t i;
    uint8_t *data = c;
    if (r->fill_cnt >= r->size)
        return -1; // ring buffer is full, this should be atomic operation


    r->fill_cnt += r->block_size;             // increase filled slots count, this should be atomic operation

    for (i = 0; i < r->block_size; i++) {
        *r->p_w = *data;              // put character into buffer

        r->p_w ++;
        data ++;
    }

    if (r->p_w >= r->p_o + r->size)     // rollback if write pointer go pass
        r->p_w = r->p_o;          // the physical boundary

    return 0;
}
/**
* \brief get a character from ring buffer
* \param r pointer to a ringbuf object
* \param c read character
* \return 0 if successfull, otherwise failed
*/
int32_t mqtt_rb_get(RINGBUF *r, uint8_t *c)
{
    int32_t i;
    uint8_t *data = c;
    if (r->fill_cnt <= 0)return -1;     // ring buffer is empty, this should be atomic operation

    r->fill_cnt -= r->block_size;              // decrease filled slots count

    for (i = 0; i < r->block_size; i++)
        *data++ = *r->p_r++;               // get the character out

    if (r->p_r >= r->p_o + r->size)       // rollback if write pointer go pass
        r->p_r = r->p_o;            // the physical boundary

    return 0;
}

int32_t mqtt_rb_available(RINGBUF *r)
{
    return (r->size - r->fill_cnt);
}

uint32_t mqtt_rb_read(RINGBUF *r, uint8_t *buf, int len)
{
    int n = 0;
    uint8_t data;
    while (len > 0) {
        while (mqtt_rb_get(r, &data) != 0);
        *buf++ = data;
        n ++;
        len --;
    }

    return n;
}

uint32_t mqtt_rb_write(RINGBUF *r, uint8_t *buf, int len)
{
    uint32_t wi;
    for (wi = 0; wi < len; wi++) {
        while (mqtt_rb_put(r, &buf[wi]) != 0);
    }
    return 0;
}
