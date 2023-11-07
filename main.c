//------------------------------------------------------------------------------
/**
 * @file main.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief iperf3 control app for odroid jig.
 * @version 0.2
 * @date 2023-11-07
 *
 * @package apt install iperf3, nmap, ethtool, usbutils, alsa-utils
 *
 * @copyright Copyright (c) 2022
 *
 */
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <getopt.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>

//------------------------------------------------------------------------------
#include "socket_server.h"
#include "socket_client.h"

//------------------------------------------------------------------------------
#define IPERF3_FILE_PATH    "/usr/bin/iperf3"
#define CMD_LINE_MAX        1024

char IPERF_CMD[CMD_LINE_MAX] = {0, };

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// 문자열 변경 함수. 입력 포인터는 반드시 메모리가 할당되어진 변수여야 함.
//------------------------------------------------------------------------------
static void tolowerstr (char *p)
{
    int i, c = strlen(p);

    for (i = 0; i < c; i++, p++)
        *p = tolower(*p);
}

//------------------------------------------------------------------------------
static void toupperstr (char *p)
{
    int i, c = strlen(p);

    for (i = 0; i < c; i++, p++)
        *p = toupper(*p);
}

//------------------------------------------------------------------------------
static void print_usage (const char *prog)
{
    /* 기존 iperf3의 경우 client의 여러 접속의 요청시 iperf3동작을 하지 않는 경우 발생 */
    /* 소켓통신을 통하여 우선순위를 할당하여 iperf3를 실행하도록 조정함. (ODROID-JIG 발생되는 문제점) */
    puts("");
    printf("Usage: %s [-s:server | -c:client {server ip}] [-p:port] [-r:retry time] [-d:delay] [-R:reverse]\n", prog);
    puts("\n"
         "  -s --server     Server mode\n"
         "  -c --client     Client mode (req : server ip sddr)\n"
         "  -p --port       TCP/IP message control port\n"
         "  -r --retry      Connect retry count\n"
         "  -d --delay(ms)  Retry wait delay\n"
         "  -R --reverse    Client mode only(server sendsm client receives)\n"
         "\n"
         "  - IPERF3 Server mode.\n"
         "      iperf3_odroid -s -p 1234\n"
         "  - IPERF3 client mode.\n"
         "      iperf3_odroid -c 192.168.0.2 -p 1234 -r 10 -d 1000\n"
    );
    puts("");
    exit(1);
}

//------------------------------------------------------------------------------
/* Control variable */
//------------------------------------------------------------------------------
static char *OPT_SERVER_IP      = NULL;
static int  OPT_SERVER_MODE     = 0;
static int  OPT_CONTROL_PORT    = 0;
static int  OPT_RETRY_DELAY     = 0;
static int  OPT_RETRY_COUNT     = 0;
static int  OPT_SERVER_REVERSE  = 0;

//------------------------------------------------------------------------------
static void parse_opts (int argc, char *argv[])
{
    while (1) {
        static const struct option lopts[] = {
            { "server"  ,  0, 0, 's' },
            { "client"  ,  1, 0, 'c' },
            { "port"    ,  1, 0, 'p' },
            { "retry"   ,  1, 0, 'r' },
            { "delay"   ,  1, 0, 'd' },
            { "reverse" ,  0, 0, 'R' },
            { NULL, 0, 0, 0 },
        };
        int c;

        c = getopt_long(argc, argv, "sc:p:r:d:R", lopts, NULL);

        if (c == -1)
            break;

        switch (c) {
            case 's':   OPT_SERVER_MODE     = 1;                break;
            case 'c':   OPT_SERVER_IP       = optarg;           break;
            case 'p':   OPT_CONTROL_PORT    = atoi (optarg);    break;
            case 'r':   OPT_RETRY_COUNT     = atoi (optarg);    break;
            case 'd':   OPT_RETRY_DELAY     = atoi (optarg);    break;
            case 'R':   OPT_SERVER_REVERSE  = 1;                break;
            default:    print_usage(argv[0]);                   break;
        }
    }
}

//------------------------------------------------------------------------------
int main (int argc, char *argv[])
{
    // iperf3 install확인
    if (access (IPERF3_FILE_PATH, R_OK) != 0) {
        printf ("Error : iperf3 is not installed. This program requires iperf3.");
        exit (1);
    }

    if (argc < 4)
        print_usage(argv[0]);

    parse_opts(argc, argv);

    if (OPT_SERVER_MODE)
        socket_server (OPT_CONTROL_PORT);
    else
        socket_client (OPT_CONTROL_PORT, OPT_SERVER_IP);

    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
