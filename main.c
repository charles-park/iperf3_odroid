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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
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
//------------------------------------------------------------------------------
static void ip_str2int (char *ip_str, int *ip_int)
{
    char *ptr, _ip_str[20];

    memset (_ip_str, 0, sizeof(_ip_str));
    strncpy(_ip_str, ip_str, strlen(ip_str));

    // ip_str = aaa.bbb.ccc.ddd
    if ((ptr = strtok(_ip_str, ".")) != NULL)   ip_int[0] = atoi(ptr);
    if ((ptr = strtok(   NULL, ".")) != NULL)   ip_int[1] = atoi(ptr);
    if ((ptr = strtok(   NULL, ".")) != NULL)   ip_int[2] = atoi(ptr);
    if ((ptr = strtok(   NULL, ".")) != NULL)   ip_int[3] = atoi(ptr);
}

//------------------------------------------------------------------------------
static void ip_int2str (int *ip_int, char *ip_str)
{
    sprintf (ip_str, "%d.%d.%d.%d", ip_int[0], ip_int[1], ip_int[2], ip_int[3]);
}

//------------------------------------------------------------------------------
static int ip_band_check (char *ip_str1, char *ip_str2)
{
    int ip_int1[4], ip_int2[4];
    char _ip_str1[20], _ip_str2[20];

    memset (ip_int1, 0, sizeof(ip_int1));
    memset (ip_int2, 0, sizeof(ip_int2));

    memset (_ip_str1, 0, sizeof(_ip_str1));
    memset (_ip_str2, 0, sizeof(_ip_str2));
    strncpy(_ip_str1, ip_str1, strlen(ip_str1));
    strncpy(_ip_str2, ip_str2, strlen(ip_str2));

    ip_str2int (_ip_str1, ip_int1);
    ip_str2int (_ip_str2, ip_int2);

    if (ip_int1[0] != ip_int2[0])   return 0;
    if (ip_int1[1] != ip_int2[1])   return 0;
    if (ip_int1[2] != ip_int2[2])   return 0;
    return 1;
}

//------------------------------------------------------------------------------
static int get_eth0_ip (char *ip_str)
{
    int fd;
    struct ifreq ifr;
    char if_info[20], *p_str;

    /* this entire function is almost copied from ethtool source code */
    /* Open control socket. */
    if ((fd = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf ("%s : Cannot get control socket\n", __func__);
        return 0;
    }
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);
    if (ioctl (fd, SIOCGIFADDR, &ifr) < 0) {
        printf ("iface name = eth0, SIOCGIFADDR ioctl Error!!\n");
        close (fd);
        return 0;
    }
    // board(iface) ip
    memset (if_info, 0, sizeof(if_info));
    inet_ntop (AF_INET, ifr.ifr_addr.sa_data+2, if_info, sizeof(struct sockaddr));

    /* aaa.bbb.ccc.ddd 형태로 저장됨 (16 bytes) */
    memcpy (ip_str, if_info, strlen(if_info));

    if ((p_str = strtok (if_info, ".")) != NULL) {
        strtok (NULL, "."); strtok (NULL, ".");

        if ((p_str = strtok (NULL, ".")) != NULL)
            return atoi (p_str);
    }
    return 0;
}

//------------------------------------------------------------------------------
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

    if (OPT_SERVER_MODE) {
        socket_server (OPT_CONTROL_PORT);
    }
    else {
        char ip_str[20];

        memset (ip_str, 0, sizeof(ip_str));
        // Client의 경우 같은 대역의 IP주소를 같고 있는지 확인.
        // iperf3실행시 같은 대역이 아닌경우 무한 loop에 빠지는 현상이 발생함.
        if (get_eth0_ip (ip_str)) {
            if (!ip_band_check (OPT_SERVER_IP, ip_str)) {
                printf ("Error : IP band. board ip = %s, server ip = %s\n", ip_str, OPT_SERVER_IP);
                exit (1);
            }
        }
        do {
            if (socket_client (OPT_CONTROL_PORT, OPT_SERVER_IP, OPT_SERVER_REVERSE))
                break;
            if (OPT_RETRY_COUNT)
                OPT_RETRY_COUNT--;
            if (OPT_RETRY_DELAY)
                usleep (OPT_RETRY_DELAY * 1000);
        } while (OPT_RETRY_COUNT);
    }

    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
