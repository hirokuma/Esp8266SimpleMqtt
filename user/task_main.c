#include <string.h>

#include "esp_common.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "user_config.h"

#include "task_main.h"
#include "task_smartconfig.h"


static void connect_ap(void);
static int connect_tcp(const char *pAddr, const char *pPort);
static bool mqtt_send_connect(const char *pClient);
static bool mqtt_send_publish(const char *pTopic, const char *pMsg);


static int mSock;


void task_main(void *pvParameters)
{
    bool ret = false;

    connect_ap();

    // https://iot.eclipse.org/getting-started
    mSock = connect_tcp("iot.eclipse.org", "1883");
    if (mSock >= 0) {
        //OK
        ret = mqtt_send_connect("my_client");
        printf("mqtt connect : %d\n", ret);
    }
    if (ret) {
        //MQTT connected
        while (true) {
            //10秒ごとにpublish
            mqtt_send_publish("my_topic", "hello");
            vTaskDelay(10000 / portTICK_RATE_MS);
        }
    }

    vTaskDelete(NULL);
}


static void connect_ap(void)
{
    bool bFirst = true;

    while (true) {
        //1秒ごとにチェック
        for (int lp = 0; lp < 30; lp++) {
            STATION_STATUS st = wifi_station_get_connect_status();
            if (st == STATION_GOT_IP) {
                goto LABEL_EXIT;
            }
            printf(".");
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
        if (bFirst) {
            //見つからなかったので、smart configモードに映る
            printf("start SmartConfig\n");
            xTaskCreate(task_smartconfig, "task_smartconfig", 256, NULL, 2, NULL);
            bFirst = false;
        }
    }

LABEL_EXIT:
    printf("AP connected.\n");
}


static int connect_tcp(const char *pAddr, const char *pPort)
{
    int sock = -1;
    struct addrinfo hints;
    struct addrinfo *ainfo;

    //IPアドレス取得
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;

    int ret = getaddrinfo(pAddr, pPort, &hints, &ainfo);
    if (!ret) {
        struct addrinfo *rp;
        for (rp = ainfo; rp != NULL; rp = rp->ai_next) {
            printf("TCP connect start\n");
            sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (sock == -1) {
                printf("fail: sock\n");
                goto LABEL_EXIT;
            }
            printf("socket ok\n");

            struct sockaddr_in *in = (struct sockaddr_in *)rp->ai_addr;
            printf("addr : %s\n", inet_ntoa(in->sin_addr));
            ret = connect(sock, rp->ai_addr, rp->ai_addrlen);
            if (!ret) {
                //接続OK
                printf("tcp connected : %d\n", sock);
                goto LABEL_EXIT;
            } else {
                close(sock);
                sock = -1;
            }
        }
        freeaddrinfo(ainfo);
        if (rp == NULL) {
            goto LABEL_EXIT;
        }
    } else {
        goto LABEL_EXIT;
    }

LABEL_EXIT:
    return sock;
}


static bool mqtt_send_connect(const char *pClient)
{
    bool ret = false;
    uint8_t *data;
    int pos = 0;

    int len_cli = strlen(pClient);
    int len_all = 12 + 2 + len_cli;
    data = (uint8_t *)malloc(2 + len_all);

    //fixed header
    data[pos++] = (0x01) << 4;
    data[pos++] = (uint8_t)len_all;
    //variable header
    data[pos++] = 0;            //len
    data[pos++] = 6;
    data[pos++] = 'M';          //magic number
    data[pos++] = 'Q';
    data[pos++] = 'I';
    data[pos++] = 's';
    data[pos++] = 'd';
    data[pos++] = 'p';
    data[pos++] = 3;            //protocol version
    data[pos++] = 0x02;         //no-user/no-passwd/no-retain/QoS=0/no-will/clean-session
    data[pos++] = 0x00;         //keep alive timer
    data[pos++] = 0xff;
    //payload
    data[pos++] = 0;            //cliend id len
    data[pos++] = (uint8_t)len_cli;
    memcpy(&data[pos], pClient, len_cli);

    ssize_t sz = write(mSock, data, 2 + len_all);
    if (sz != 2 + len_all) {
        goto LABEL_EXIT;
    }

    uint8_t buf[256];
    sz = 0;
    while (true) {
        sz += read(mSock, buf + sz, sizeof(buf) - sz);
        if (sz >= 4) {
            break;
        }
    }
    if (((buf[0] & 0xf0) == 0x20) && (buf[1] == 2) && (buf[3] == 0x00)) {
        //CONNACK
        ret = true;
    }

LABEL_EXIT:
    free(data);
    return ret;
}


static bool mqtt_send_publish(const char *pTopic, const char *pMsg)
{
    bool ret = false;
    uint8_t *data;
    int pos = 0;

    int len_topic = strlen(pTopic);
    int len_msg = strlen(pMsg);
    int len_all = 2 + len_topic + 2 + len_msg;
    data = (uint8_t *)malloc(2 + len_all);

    //fixed header
    data[pos++] = (0x03) << 4;
    data[pos++] = (uint8_t)len_all;
    //valiable header
    data[pos++] = 0;                //topic len
    data[pos++] = (uint8_t)len_topic;
    memcpy(&data[pos], pTopic, len_topic);
    pos += len_topic;
    //payload
    data[pos++] = 0;                //message id len
    data[pos++] = (uint8_t)len_msg;
    memcpy(&data[pos], pMsg, len_msg);  //message

    ssize_t sz = write(mSock, data, 2 + len_all);
    printf("write=%d\n", sz);

    free(data);
    return sz == 2 + len_all;
}
