/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
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

#include "esp_common.h"

#include "task_smartconfig.h"


static void smartconfig_done(sc_status status, void *pdata)
{
    switch(status) {
    case SC_STATUS_WAIT:
        printf("SC_STATUS_WAIT\n");
        break;
    case SC_STATUS_FIND_CHANNEL:
        printf("SC_STATUS_FIND_CHANNEL\n");
        break;
    case SC_STATUS_GETTING_SSID_PSWD:
        printf("SC_STATUS_GETTING_SSID_PSWD\n");
        sc_type *type = pdata;
        if (*type == SC_TYPE_ESPTOUCH) {
            printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
        } else {
            printf("SC_TYPE:SC_TYPE_AIRKISS\n");
            smartconfig_stop();
        }
        break;
    case SC_STATUS_LINK:
        printf("SC_STATUS_LINK\n");
        struct station_config *sta_conf = pdata;

        //
        wifi_station_set_config(sta_conf);
        wifi_station_disconnect();
        wifi_station_connect();
        break;
    case SC_STATUS_LINK_OVER:
        printf("SC_STATUS_LINK_OVER\n");
        if (pdata != NULL) {
            //SC_TYPE_ESPTOUCH
            uint8 phone_ip[4] = {0};

            memcpy(phone_ip, (uint8*)pdata, 4);
            printf("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);
        }
        smartconfig_stop();
        break;
    }

}


void task_smartconfig(void *pvParameters)
{
    smartconfig_set_type(SC_TYPE_ESPTOUCH);
    smartconfig_start(smartconfig_done);
    vTaskDelete(NULL);
}
