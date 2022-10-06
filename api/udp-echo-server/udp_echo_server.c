#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <time.h>

#include "udp_echo_server.h"

/*---------------------------------------------------------------------------*/

int main(int argc, char **  argv)
{
    if (BUF_SIZE != 2048)
        printf("\033[1;31mBuffer size {%ld} isn't equal to 2048\n\033[0m",
               BUF_SIZE);

    int sockfd;
    int ret;
    char recv_buf[BUF_SIZE];
    unsigned int len;
    unsigned int arg_bit;
    struct sockaddr_in servaddr, cliaddr;
    struct args args;
    struct d_time start, end;
    struct addrs_info addrs_info;

    /*-----------------------------------------------------------------------*/

    arg_bit = get_args(argc, argv);
    args = parse_args(argc, argv, arg_bit);

    /*-----------------------------------------------------------------------*/

    /* creating socket file descriptor */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    /*-----------------------------------------------------------------------*/

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    /*-----------------------------------------------------------------------*/

    /* filling server information */
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port        = htons(args.src_port);

    /*-----------------------------------------------------------------------*/

    /* bind the socket with the server address */
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Server created on %s:%d\n", "0.0.0.0", args.src_port);
    printf("Resend to %s:%d\n", args.dst_ip, args.dst_port);

    /*-----------------------------------------------------------------------*/

    addrs_info.print = print_net_info;
    addrs_info.args  = &args;
    show_statistic((void *) &addrs_info);

    /*-----------------------------------------------------------------------*/

    /* clear time structures */
    clear_dtime(&start);
    clear_dtime(&end);

    /*-----------------------------------------------------------------------*/

    len = sizeof(cliaddr);

    while(1)
    {
        /* start clock */
        get_time(&start);

        /* get UDP packet */
        ret = recvfrom(sockfd, (char *)recv_buf, BUF_SIZE, 0,
                       (struct sockaddr *) &cliaddr, &len);

        if (ret != BUF_SIZE)
        {
            printf("received from socket less than expected: %d\n",
                   ret);
            exit(EXIT_FAILURE);
        }
        
        pkg_recv++;

        /*-------------------------------------------------------------------*/

        /* when timeout is set */
        if (args.timeout)
            usleep(args.timeout);

        /*-------------------------------------------------------------------*/

        /* set client addr and port */
        cliaddr.sin_port = htons(args.dst_port);
        cliaddr.sin_addr.s_addr = inet_addr(args.dst_ip);

        /* send message over UDP back */
        ret = sendto(sockfd, (const char *)recv_buf, BUF_SIZE, 0,
               (const struct sockaddr *) &cliaddr, len);

        if (ret != BUF_SIZE)
        {
            perror("sent to UDP less than needed\n");
            exit(1);
        }

        pkg_sent++;

        /* stop clock */
        get_time(&end);

        /* count duration without first packet */
        if (pkg_sent != 1)
            duration += end.diff(&start, &end);
    }

    /*-----------------------------------------------------------------------*/

    return 0;
}
