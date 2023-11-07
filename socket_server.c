//------------------------------------------------------------------------------
/**
 * @file socket_server.c
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
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include "socket_server.h"

#define CMD_LINE_MAX    1024

//------------------------------------------------------------------------------
static int ethernet_iperf3 (void)
{
    FILE *fp;
    char cmd_line[CMD_LINE_MAX];
    int value = 0;

    if ((fp = popen("iperf3 -s -1", "r")) != NULL) {
        while (1) {
            memset (cmd_line, 0, sizeof (cmd_line));
            if (fgets (cmd_line, sizeof (cmd_line), fp) == NULL)
                break;

            if ((strstr (cmd_line, "sender") != NULL) || (strstr (cmd_line, "receiver") != NULL)) {
                value = 1;
                break;
            }
        }
        pclose(fp);
    }
    return value;
}

//------------------------------------------------------------------------------
static int server_loop (int s_fd)
{
    struct sockaddr_in addr;
    int n_socket, r_cnt, len = sizeof(addr);
    char buf[CMD_LINE_MAX] = {0,};

    memset (buf, 0, sizeof(buf));

    while (1) {
        if ((n_socket = accept (s_fd, (struct sockaddr *)&addr, (socklen_t *)&len)) < 0) {
            printf ("Error : accept\n");
            return 0;
        }

        printf ("connect ip : %s\n", inet_ntoa(addr.sin_addr));

        if ((r_cnt = read (n_socket, buf, sizeof(buf)))) {
            if (!strncmp("iperf3", buf, strlen("iperf3")-1)) {
                send (n_socket, "iperf3_run", strlen("iperf3_run"), 0);
                printf ("run iperf3 server command. ret = %d\n", ethernet_iperf3 ());
                close (n_socket);
            }
        }
        memset (buf, 0, sizeof(buf));
        usleep(1000);
    }

    return 1;

}

//------------------------------------------------------------------------------
int socket_server (int port)
{
    int s_fd, opt = 1;
    struct sockaddr_in addr;

    if ((s_fd = socket (AF_INET, SOCK_STREAM,0)) == 0) {
        printf ("Error : Socket creating error!\n");
        goto err_out;
    }

    if (setsockopt (s_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        printf ("Error : setsockopt error\n");
        goto err_out;
    }

    addr.sin_family         = AF_INET;
    addr.sin_addr.s_addr    = INADDR_ANY;
    addr.sin_port           = htons (port);

    if (bind(s_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf ("Error : bind failed\n");
        goto err_out;
    }

    if (listen (s_fd, BACK_LOG_COUNT) < 0) {
        printf ("Error : listen\n");
        goto err_out;
    }
    return server_loop (s_fd);

err_out:
    if (s_fd)   close (s_fd);
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
