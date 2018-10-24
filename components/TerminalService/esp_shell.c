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

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_shell.h"
#include "driver/uart.h"
#define uart_num UART_NUM_0

static ShellCommand *scp = NULL;
static int shell_run = 0;

static char *parse_arguments(char *str, char **saveptr) {
    char *p;

    if (str != NULL)
        *saveptr = str;

    p = *saveptr;
    if (!p) {
        return NULL;
    }

    /* Skipping white space.*/
    p += strspn(p, " \t");

    if (*p == '"') {
        /* If an argument starts with a double quote then its delimiter is another
           quote.*/
        p++;
        *saveptr = strpbrk(p, "\"");
    }
    else {
        /* The delimiter is white space.*/
        *saveptr = strpbrk(p, " \t");
    }

    /* Replacing the delimiter with a zero.*/
    if (*saveptr != NULL) {
        *(*saveptr)++ = '\0';
    }

    return *p != '\0' ? p : NULL;
}



static void list_commands(const ShellCommand *scp) {
    ShellCommand *_scp = scp;
    while (_scp->sc_name != NULL) {
        printf("%s \r\n", _scp->sc_name);
        _scp++;
    }
}


static bool cmdexec(void *ref, char *name, int argc, char *argv[]) {
    ShellCommand *_scp = scp;
    while (_scp->sc_name != NULL) {
        if (strcmp(_scp->sc_name, name) == 0) {
            _scp->sc_function(ref, argc, argv);
            return false;
        }
        _scp++;
    }
    return true;
}

bool shellGetLine(char *line, unsigned size) {
    char *p = line;
    char c;
    char tx[3];
    while (true) {
        uart_read_bytes(uart_num, (uint8_t*)&c, 1, portMAX_DELAY);

        if ((c == 8) || (c == 127)) {  // backspace or del
            if (p != line) {
                tx[0] = c;
                tx[1] = 0x20;
                tx[2] = c;
                uart_write_bytes(uart_num, (const char*)tx, 3);
                p--;
            }
            continue;
        }
        if (c == '\n' || c == '\r') {
            tx[0] = c;
            uart_write_bytes(uart_num, (const char*)tx, 1);
            *p = 0;
            return false;
        }

        if (c < 0x20)
            continue;
        if (p < line + size - 1) {
            tx[0] = c;
            uart_write_bytes(uart_num, (const char*)tx, 1);
            *p++ = (char)c;
        }
    }
}


void shell_task(void *pv)
{
    int n;

    char *lp, *cmd, *tokp, line[SHELL_MAX_LINE_LENGTH];
    char *args[SHELL_MAX_ARGUMENTS + 1];


    while (shell_run) {
        printf(">");

        if (shellGetLine(line, sizeof(line))) {

        }

        lp = parse_arguments(line, &tokp);
        cmd = lp;
        n = 0;
        while ((lp = parse_arguments(NULL, &tokp)) != NULL) {
            if (n >= SHELL_MAX_ARGUMENTS) {
                printf("too many arguments"SHELL_NEWLINE_STR);
                cmd = NULL;
                break;
            }
            args[n++] = lp;
        }
        args[n] = NULL;
        if (cmd != NULL) {
            if (cmdexec(pv, cmd, n, args)) {
                printf("%s", cmd);
                printf("?"SHELL_NEWLINE_STR);
                list_commands(scp);
            }
        }

    }
    vTaskDelete(NULL);
}

void shell_init(const ShellCommand *_scp, void *ref)
{
    shell_run = 1;
    scp = _scp;

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };
    uart_param_config(uart_num, &uart_config);
    uart_driver_install(uart_num,  256, 0, 0, NULL, 0);
    xTaskCreatePinnedToCore(&shell_task, "shell", 4096, ref, 3, NULL, xPortGetCoreID());
}

void shell_stop()
{
    shell_run = 0;
}
