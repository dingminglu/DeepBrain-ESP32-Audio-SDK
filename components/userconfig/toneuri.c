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

/*This is tone file*/

const char* tone_uri[] = {
   "flsh:///0_Bt_Reconnect.mp3",
   "flsh:///1_Wechat.mp3",
   "flsh:///2_Welcome_To_Wifi.mp3",
   "flsh:///3_New_Version_Available.mp3",
   "flsh:///4_Bt_Success.mp3",
   "flsh:///5_Freetalk.mp3",
   "flsh:///6_Upgrade_Done.mp3",
   "flsh:///7_shutdown.mp3",
   "flsh:///8_Alarm.mp3",
   "flsh:///9_Wifi_Success.mp3",
   "flsh:///10_Under_Smartconfig.mp3",
   "flsh:///11_Out_Of_Power.mp3",
   "flsh:///12_server_connect.mp3",
   "flsh:///13_hello.mp3",
   "flsh:///14_Please_Retry_Wifi.mp3",
   "flsh:///15_please_setting_wifi.mp3",
   "flsh:///16_Welcome_To_Bt.mp3",
   "flsh:///17_Wifi_Time_Out.mp3",
   "flsh:///18_Wifi_Reconnect.mp3",
   "flsh:///19_server_disconnect.mp3",
};

int getToneUriMaxIndex()
{
    return sizeof(tone_uri) / sizeof(char*) - 1;
}
