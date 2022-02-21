#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>

#include "udp_dma_client.h"

/*---------------------------------------------------------------------------*/

#define OWN_PORT  7761

#define BACK_IP   "192.168.11.203"
#define BACK_PORT 7766

int pkg = 0;

/*---------------------------------------------------------------------------*/
  
int main(int __attribute__ ((unused)) argc,
         char ** __attribute__ ((unused)) argv)
{
    int sockfd;
    int fd;
    int ret;
    char recv_buf[BUF_SIZE];
    char send_buf[BUF_SIZE];
    unsigned int len;
    struct sockaddr_in servaddr, cliaddr;
    const char * file_name = "/dev/dma_elixir_o_v1";

    /*-----------------------------------------------------------------------*/
    
    /* add arguments for dst, src, and so on */

    /*-----------------------------------------------------------------------*/
      
    // Creating socket file descriptor
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
    servaddr.sin_port        = htons(OWN_PORT);

    /*-----------------------------------------------------------------------*/
      
    /* bind the socket with the server address */
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
        
    printf("Server created on %s:%d\n", "0.0.0.0", OWN_PORT);
    printf("Resend to %s:%d\n", BACK_IP, BACK_PORT);

    /*-----------------------------------------------------------------------*/

    /* open device file for read/write */
    fd = open(file_name, O_RDWR);
    if (fd < 0)
    {
        perror("can't open device file");
        exit(EXIT_FAILURE);
    }

    /*-----------------------------------------------------------------------*/

    len = sizeof(cliaddr);

    while(1)
    {  
        /* get UDP packet */
        ret = recvfrom(sockfd, (char *)recv_buf, BUF_SIZE, 0,
                       (struct sockaddr *) &cliaddr, &len);

        if (ret != BUF_SIZE)
        {
            perror("received from socket less than expected\n");
            printf("%d\n%d\n", ret, pkg);
            exit(EXIT_FAILURE);
        }

        pkg++;

        /*-------------------------------------------------------------------*/

        /* write UDP packet to DMA driver */
        ret = write(fd, recv_buf, BUF_SIZE);
        if (ret != BUF_SIZE)
        {
            perror("writed to device file less than needed\n");
            exit(1);
        }

        /*-------------------------------------------------------------------*/

        /* read from DMA driver */
        ret = read(fd, send_buf, BUF_SIZE);
        if (ret != BUF_SIZE)
        {
            perror("received from device file less than needed\n");
            exit(1);
        }

        /*-------------------------------------------------------------------*/

        /* set client addr and port */
        cliaddr.sin_port = htons(BACK_PORT);
        cliaddr.sin_addr.s_addr = inet_addr(BACK_IP);

        /* send message over UDP back */
        ret = sendto(sockfd, (const char *)send_buf, BUF_SIZE, 0,
               (const struct sockaddr *) &cliaddr, len);

        if (ret != BUF_SIZE)
        {
            perror("sent to UDP less than needed\n");
            exit(1);
        }
    }

    /*-----------------------------------------------------------------------*/

    return 0;
}
