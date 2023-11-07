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

#define CMD_LINE_MAX    1024

//------------------------------------------------------------------------------
static int ethernet_iperf3 (const char *s_ip, const char *found_str)
{
    FILE *fp;
    char cmd_line[CMD_LINE_MAX], *pstr;
    int value = 0;

    memset (cmd_line, 0x00, sizeof(cmd_line));
    if (!strncmp (found_str, "sender", strlen("sender")-1))
        sprintf(cmd_line, "iperf3 -t 1 -c %s", s_ip);
    else
        sprintf(cmd_line, "iperf3 -t 1 -R -c %s", s_ip);

    if ((fp = popen(cmd_line, "r")) != NULL) {
        while (1) {
            memset (cmd_line, 0, sizeof (cmd_line));
            if (fgets (cmd_line, sizeof (cmd_line), fp) == NULL)
                break;

            if (strstr (cmd_line, found_str) != NULL) {
                if ((pstr = strstr (cmd_line, "MBytes")) != NULL) {
                    while (*pstr != ' ')    pstr++;
                    value = atoi (pstr);
                }
            }
        }
        pclose(fp);
    }
    return value;
}

//------------------------------------------------------------------------------
static int client_loop (int s_fd, char *s_ip, int reverse)
{
    int r_cnt, mbits;
    char buf[CMD_LINE_MAX] = {0,};

    memset (buf, 0, sizeof(buf));
    // iperf3 server call
    send (s_fd, "iperf3", strlen("iperf3"), 0);

    while (1) {
        if ((r_cnt = read (s_fd, buf, sizeof(buf)))) {
            if (!strncmp (buf, "iperf3_run", strlen("iperf3_run")-1)) {
		usleep (100 * 1000);
                mbits = ethernet_iperf3 (s_ip, reverse ? "receiver" : "sender");
                // iperf3 result
                printf ("MBytes %d Mbits/sec %s\n", mbits, reverse ? "receiver" : "sender");
                close (s_fd);
                return mbits ? 1 : 0;
            }
            memset (buf, 0, sizeof(buf));
        }
        usleep (1000);
    }
    return 0;
}

//------------------------------------------------------------------------------
int socket_client (int port, char *s_ip, int reverse)
{
    int s_fd;
    struct sockaddr_in addr;

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

    return  client_loop(s_fd, s_ip, reverse);

err_out:
    if (s_fd)   close (s_fd);
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
