//------------------------------------------------------------------------------
/**
 * @file socket_client.c
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
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include "socket_client.h"

//------------------------------------------------------------------------------
int socket_client (int port, char *s_ip)
{
    int s_fd, r_cnt;

    struct sockaddr_in addr;

    char buf[1024] = {0,};

    if ((s_fd = socket (AF_INET, SOCK_STREAM,0)) == 0) {
        printf ("Error : Socket creating error!\n");
        goto err_out;
    }

    addr.sin_family = AF_INET;
    addr.sin_port   = htons (port);

    if (inet_pton(AF_INET, s_ip, &addr.sin_addr) <= 0) {
        printf ("Error : Invalid address / Address not supported.\n");
        goto err_out;
    }

    if (connect (s_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf ("Error : server connect failed\n");
        goto err_out;
    }

    // loop start
    send (s_fd, "hello", strlen("hello"), 0);

    printf ("%s : hello message sent\n", __func__);
    r_cnt = read (s_fd, buf, sizeof(buf));

    if (r_cnt)
        printf ("%s(%d) : %s\n", __func__, r_cnt, buf);

    close (s_fd);
    // loop end
    return 1;

err_out:
    if (s_fd)   close (s_fd);
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
