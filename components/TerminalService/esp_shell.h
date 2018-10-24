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

#ifndef _ESP_SHELL_H_
#define _ESP_SHELL_H_

typedef void (*shellcmd_t)(void *self, int argc, char *argv[]);

/**
 * @brief   Custom command entry type.
 */
typedef struct {
    const char      *sc_name;           /**< @brief Command name.       */
    shellcmd_t      sc_function;        /**< @brief Command function.   */
} ShellCommand;


#define _shell_clr_line(stream)   chprintf(stream, "\033[K")

#define SHELL_NEWLINE_STR   "\r\n"
#define SHELL_MAX_ARGUMENTS 10
#define SHELL_PROMPT_STR "\r\n>"
#define SHELL_MAX_LINE_LENGTH 512

void shell_init(const ShellCommand *_scp, void *ref);
void shell_stop();
#endif
