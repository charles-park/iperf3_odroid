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

//------------------------------------------------------------------------------
int socket_server (int port)
{
    int s_fd, n_socket, r_cnt;

    struct sockaddr_in addr;

    int opt = 1;
    int len = sizeof(addr);
    char buf[1024] = {0,};

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

    if ((n_socket = accept (s_fd, (struct sockaddr *)&addr, (socklen_t *)&len)) < 0) {
        printf ("Error : accept\n");
        goto err_out;
    }

    // loop
    r_cnt = read (n_socket, buf, sizeof(buf));

    if (r_cnt)
        printf ("%s(%d) : %s\n", __func__, r_cnt, buf);

    send (n_socket, "hello", strlen("hello"), 0);
    printf ("%s : hello message sent\n", __func__);

    close (s_fd);
    // loop end

    return 1;
err_out:
    if (s_fd)   close (s_fd);
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
